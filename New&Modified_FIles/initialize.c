/* initialize.c - nulluser, sizmem, sysinit */

#include <conf.h>
#include <i386.h>
#include <kernel.h>
#include <proc.h>
#include <sem.h>
#include <sleep.h>
#include <mem.h>
#include <tty.h>
#include <q.h>
#include <io.h>
#include <paging.h>

/*#define DETAIL */
#define HOLESIZE	(600)	
#define	HOLESTART	(640 * 1024)
#define	HOLEEND		((1024 + HOLESIZE) * 1024)  
/* Extra 600 for bootp loading, and monitor */

extern	int	main();	/* address of user's main prog	*/

extern	int	start();

LOCAL		sysinit();

/* Declarations of major kernel variables */
struct	pentry	proctab[NPROC]; /* process table			*/
int	nextproc;		/* next process slot to use in create	*/
struct	sentry	semaph[NSEM];	/* semaphore table			*/
int	nextsem;		/* next sempahore slot to use in screate*/
struct	qent	q[NQENT];	/* q table (see queue.c)		*/
int	nextqueue;		/* next slot in q structure to use	*/
char	*maxaddr;		/* max memory address (set by sizmem)	*/
struct	mblock	memlist;	/* list of free memory blocks		*/
#ifdef	Ntty
struct  tty     tty[Ntty];	/* SLU buffers and mode control		*/
#endif

/* active system status */
int	numproc;		/* number of live user processes	*/
int	currpid;		/* id of currently running process	*/
int	reboot = 0;		/* non-zero after first boot		*/

int	rdyhead,rdytail;	/* head/tail of ready list (q indicies)	*/
char 	vers[80];
int	console_dev;		/* the console device			*/

/*  added for the demand paging */
int page_replace_policy = SC;

int global_frame_1;
int global_frame_2;
int global_frame_3;
int global_frame_4;

pd_t *init_pd(int pid) {
	// init global page tables (x4)
	// also init page directory
	pd_t *pd;
	// need to find an available frame
	int avail;
	get_frm(&avail);
	// allocate the frame for the page directory
	frm_tab[avail].fr_status = FRM_MAPPED;
	frm_tab[avail].fr_pid = pid;
	frm_tab[avail].fr_type = FR_DIR;
	pd = (pd_t *) (NBPG * (avail + FRAME0)); // assign address of frame to dir
	// pd->pd_pres = 1;
	// pd->pd_write = 1;
	// now allocate first four frames for the first four page tables
	int i = 0;
	if (pid == 0) {
		for (i = 0; i < 4; i++) {
			int pt_frame;
			pt_t *pt;
			get_frm(&pt_frame);
			frm_tab[pt_frame].fr_status = FRM_MAPPED;
			frm_tab[pt_frame].fr_pid = pid;
			frm_tab[pt_frame].fr_type = FR_TBL;
			frm_tab[pt_frame].fr_vpno = FRAME0 + pt_frame;
			pt = (pt_t *) (NBPG * (pt_frame + FRAME0));
			if (i == 0) {
				global_frame_1 = pt_frame;
			} else if (i == 1) {
				global_frame_2 = pt_frame;
			} else if (i == 2) {
				global_frame_3 = pt_frame;
			} else if (i == 3) {
				global_frame_4 = pt_frame;
			}
			// pt->pt_pres = 0;
			// pt->pt_write = 1;
			pd[i].pd_base = FRAME0 + pt_frame;//(((unsigned int) pt) >> 3);
			pd[i].pd_pres = 1;
			pd[i].pd_write = 1;
			pd[i].pd_user = 0;
			pd[i].pd_pwt = 0;
			pd[i].pd_pcd = 0;
			pd[i].pd_acc = 0;
			pd[i].pd_mbz = 0;
			pd[i].pd_fmb = 0;
			pd[i].pd_global = 0;
			pd[i].pd_avail = 0;
			int j = 0;
			for (j = 0; j < 1024; j++) {
				pt[j].pt_pres = 1;
				pt[j].pt_write = 1;
				pt[j].pt_user = 0;
				pt[j].pt_pwt = 0;
				pt[j].pt_pcd = 0;
				pt[j].pt_acc = 0;
				pt[j].pt_dirty = 0;
				pt[j].pt_mbz = 0;
				pt[j].pt_global = 0;
				pt[j].pt_avail = 0;
				pt[j].pt_base = (FRAME0 * i) + j; // map to the exact phsyical frame
			}
		}
		// fill in rest of entries
		for (i = 4; i < 1024; i++) {
			pd[i].pd_pres = 0;
			pd[i].pd_write = 1;
			pd[i].pd_user = 0;
			pd[i].pd_pwt = 0;
			pd[i].pd_pcd = 0;
			pd[i].pd_acc = 0;
			pd[i].pd_mbz = 0;
			pd[i].pd_fmb = 0;
			pd[i].pd_global = 0;
			pd[i].pd_avail = 0;
			pd[i].pd_base = 0;
		}
	} else {
		// just use already created pts
		for (i = 0; i < 4; i++) {
			int pt_frame = 0;
			if (i == 0) {
				pt_frame = global_frame_1;
			} else if (i == 1) {
				pt_frame = global_frame_2;
			} else if (i == 2) {
				pt_frame = global_frame_3;
			} else if (i == 3) {
				pt_frame = global_frame_4;
			}
			pt_t *pt = (pt_t *) (NBPG * (pt_frame + FRAME0));
			pd[i].pd_base = FRAME0 + pt_frame;//(((unsigned int) pt) >> 3);
			pd[i].pd_pres = 1;
			pd[i].pd_write = 1;
			pd[i].pd_user = 0;
			pd[i].pd_pwt = 0;
			pd[i].pd_pcd = 0;
			pd[i].pd_acc = 0;
			pd[i].pd_mbz = 0;
			pd[i].pd_fmb = 0;
			pd[i].pd_global = 0;
			pd[i].pd_avail = 0;
			int j = 0;
			for (j = 0; j < 1024; j++) {
				pt[j].pt_pres = 1;
				pt[j].pt_write = 1;
				pt[j].pt_user = 0;
				pt[j].pt_pwt = 0;
				pt[j].pt_pcd = 0;
				pt[j].pt_acc = 0;
				pt[j].pt_dirty = 0;
				pt[j].pt_mbz = 0;
				pt[j].pt_global = 0;
				pt[j].pt_avail = 0;
				pt[j].pt_base = (FRAME0 * i) + j; // map to the exact phsyical frame
			}
		}
		for (i = 4; i < 1024; i++) {
			pd[i].pd_pres = 0;
			pd[i].pd_write = 1;
			pd[i].pd_user = 0;
			pd[i].pd_pwt = 0;
			pd[i].pd_pcd = 0;
			pd[i].pd_acc = 0;
			pd[i].pd_mbz = 0;
			pd[i].pd_fmb = 0;
			pd[i].pd_global = 0;
			pd[i].pd_avail = 0;
			pd[i].pd_base = 0;
		}
	}
	return NBPG * (avail + FRAME0);
}

/************************************************************************/
/***				NOTE:				      ***/
/***								      ***/
/***   This is where the system begins after the C environment has    ***/
/***   been established.  Interrupts are initially DISABLED, and      ***/
/***   must eventually be enabled explicitly.  This routine turns     ***/
/***   itself into the null process after initialization.  Because    ***/
/***   the null process must always remain ready to run, it cannot    ***/
/***   execute code that might cause it to be suspended, wait for a   ***/
/***   semaphore, or put to sleep, or exit.  In particular, it must   ***/
/***   not do I/O unless it uses kprintf for polled output.           ***/
/***								      ***/
/************************************************************************/

/*------------------------------------------------------------------------
 *  nulluser  -- initialize system and become the null process (id==0)
 *------------------------------------------------------------------------
 */
nulluser()				/* babysit CPU when no one is home */
{
        int userpid;

	console_dev = SERIAL0;		/* set console to COM0 */

	initevec();

	kprintf("system running up!\n");
	sysinit();

	enable();		/* enable interrupts */

	sprintf(vers, "PC Xinu %s", VERSION);
	kprintf("\n\n%s\n", vers);
	if (reboot++ < 1)
		kprintf("\n");
	else
		kprintf("   (reboot %d)\n", reboot);


	kprintf("%d bytes real mem\n",
		(unsigned long) maxaddr+1);
#ifdef DETAIL	
	kprintf("    %d", (unsigned long) 0);
	kprintf(" to %d\n", (unsigned long) (maxaddr) );
#endif	

	kprintf("%d bytes Xinu code\n",
		(unsigned long) ((unsigned long) &end - (unsigned long) start));
#ifdef DETAIL	
	kprintf("    %d", (unsigned long) start);
	kprintf(" to %d\n", (unsigned long) &end );
#endif

#ifdef DETAIL	
	kprintf("%d bytes user stack/heap space\n",
		(unsigned long) ((unsigned long) maxaddr - (unsigned long) &end));
	kprintf("    %d", (unsigned long) &end);
	kprintf(" to %d\n", (unsigned long) maxaddr);
#endif	
	
	kprintf("clock %sabled\n", clkruns == 1?"en":"dis");


	/* create a process to execute the user's main program */
	userpid = create(main,INITSTK,INITPRIO,INITNAME,INITARGS);
	resume(userpid);

	while (TRUE)
		/* empty */;
}

/*------------------------------------------------------------------------
 *  sysinit  --  initialize all Xinu data structeres and devices
 *------------------------------------------------------------------------
 */
LOCAL
sysinit()
{
	static	long	currsp;
	int	i,j;
	struct	pentry	*pptr;
	struct	sentry	*sptr;
	struct	mblock	*mptr;
	SYSCALL pfintr();

	

	numproc = 0;			/* initialize system variables */
	nextproc = NPROC-1;
	nextsem = NSEM-1;
	nextqueue = NPROC;		/* q[0..NPROC-1] are processes */

	/* initialize free memory list */
	/* PC version has to pre-allocate 640K-1024K "hole" */
	if (maxaddr+1 > HOLESTART) {
		memlist.mnext = mptr = (struct mblock *) roundmb(&end);
		mptr->mnext = (struct mblock *)HOLEEND;
		mptr->mlen = (int) truncew(((unsigned) HOLESTART -
	     		 (unsigned)&end));
        mptr->mlen -= 4;

		mptr = (struct mblock *) HOLEEND;
		mptr->mnext = 0;
		mptr->mlen = (int) truncew((unsigned)maxaddr - HOLEEND -
	      		NULLSTK);
/*
		mptr->mlen = (int) truncew((unsigned)maxaddr - (4096 - 1024 ) *  4096 - HOLEEND - NULLSTK);
*/
	} else {
		/* initialize free memory list */
		memlist.mnext = mptr = (struct mblock *) roundmb(&end);
		mptr->mnext = 0;
		mptr->mlen = (int) truncew((unsigned)maxaddr - (int)&end -
			NULLSTK);
	}
	

	for (i=0 ; i<NPROC ; i++)	/* initialize process table */
		proctab[i].pstate = PRFREE;

	init_frm(); // initialize frame table
	init_bsm(); // initialize backing store map list
	init_queue();


#ifdef	MEMMARK
	_mkinit();			/* initialize memory marking */
#endif

#ifdef	RTCLOCK
	clkinit();			/* initialize r.t.clock	*/
#endif

	mon_init();     /* init monitor */

#ifdef NDEVS
	for (i=0 ; i<NDEVS ; i++ ) {	    
	    init_dev(i);
	}
#endif

	pptr = &proctab[NULLPROC];	/* initialize null process entry */
	pptr->pstate = PRCURR;
	for (j=0; j<7; j++)
		pptr->pname[j] = "prnull"[j];
	pptr->plimit = (WORD)(maxaddr + 1) - NULLSTK;
	pptr->pbase = (WORD) maxaddr - 3;
/*
	pptr->plimit = (WORD)(maxaddr + 1) - NULLSTK - (4096 - 1024 )*4096;
	pptr->pbase = (WORD) maxaddr - 3 - (4096-1024)*4096;
*/
	pptr->pesp = pptr->pbase-4;	/* for stkchk; rewritten before used */
	*( (int *)pptr->pbase ) = MAGIC;
	pptr->paddr = (WORD) nulluser;
	pptr->pargs = 0;
	pptr->pprio = 0;
	pptr->using_vmem = -1;
	currpid = NULLPROC;

	pptr->pdbr = (unsigned long) init_pd(NULLPROC);
	write_cr3(pptr->pdbr);

	for (i=0 ; i<NSEM ; i++) {	/* initialize semaphores */
		(sptr = &semaph[i])->sstate = SFREE;
		sptr->sqtail = 1 + (sptr->sqhead = newqueue());
	}

	rdytail = 1 + (rdyhead=newqueue());/* initialize ready list */
	
	set_evec(14, (u_long) pfintr);
	enable_paging();

	return(OK);
}

stop(s)
char	*s;
{
	kprintf("%s\n", s);
	kprintf("looping... press reset\n");
	while(1)
		/* empty */;
}

delay(n)
int	n;
{
	DELAY(n);
}


#define	NBPG	4096

/*------------------------------------------------------------------------
 * sizmem - return memory size (in pages)
 *------------------------------------------------------------------------
 */
long sizmem()
{
	unsigned char	*ptr, *start, stmp, tmp;
	int		npages;

	/* at least now its hacked to return
	   the right value for the Xinu lab backends (16 MB) */

	return 4096; 

	start = ptr = 0;
	npages = 0;
	stmp = *start;
	while (1) {
		tmp = *ptr;
		*ptr = 0xA5;
		if (*ptr != 0xA5)
			break;
		*ptr = tmp;
		++npages;
		ptr += NBPG;
		if ((int)ptr == HOLESTART) {	/* skip I/O pages */
			npages += (1024-640)/4;
			ptr = (unsigned char *)HOLEEND;
		}
	}
	return npages;
}
