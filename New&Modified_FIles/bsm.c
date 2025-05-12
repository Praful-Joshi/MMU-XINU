/* bsm.c - manage the backing store mapping*/

#include <conf.h>
#include <kernel.h>
#include <paging.h>
#include <proc.h>

bs_map_t bsm_tab[NUM_BACKING_STORES];

/*-------------------------------------------------------------------------
 * init_bsm- initialize bsm_tab
 *-------------------------------------------------------------------------
 */
SYSCALL init_bsm()
{
    STATWORD ps;
    disable(ps);
    
    int i = 0;
    // init all backing stores to unmapped
    for (i = 0; i < NUM_BACKING_STORES; i++) {
        bsm_tab[i].bs_status = BSM_UNMAPPED;
        bsm_tab[i].bs_pid = -1;
    }

    restore(ps);
    return(OK);
}

/*-------------------------------------------------------------------------
 * get_bsm - get a free entry from bsm_tab 
 *-------------------------------------------------------------------------
 */
SYSCALL get_bsm(int* avail)
{
    STATWORD ps;
    disable(ps);

    int i;

    for (i = 0; i < NUM_BACKING_STORES; i++) {
        if (bsm_tab[i].bs_status == BSM_UNMAPPED && bsm_tab[i].bs_pid == -1) {
            // set pointer to this index
            *(avail) = i;
            return(OK);
        }
    }

    restore(ps);
    return(SYSERR);
}


/*-------------------------------------------------------------------------
 * free_bsm - free an entry from bsm_tab 
 *-------------------------------------------------------------------------
 */
SYSCALL free_bsm(int i)
{
    STATWORD ps;
    disable(ps);
    
    if (i >= 0 && i <= 8 && (bsm_tab[i].bs_status == BSM_MAPPED || bsm_tab[i].bs_pid > -1)) {
        // free the entry
        bsm_tab[i].bs_status = BSM_UNMAPPED;
        bsm_tab[i].bs_pid = -1;
        restore(ps);
        return(OK);
    }

    restore(ps);
    return(SYSERR);
}

/*-------------------------------------------------------------------------
 * bsm_lookup - lookup bsm_tab and find the corresponding entry
 *-------------------------------------------------------------------------
 */
SYSCALL bsm_lookup(int pid, long vaddr, int* store, int* pageth)
{
    // This is only used for a private heap?
    STATWORD ps;
    disable(ps);
    int i = 0;
    for (i = 0; i < NUM_BACKING_STORES; i++) {
        if (i == 1) {
            // kprintf("VPNO of store is %d and page requested is %d\n", bsm_tab[i].bs_vpno, vaddr/NBPG);
            // kprintf("PID of store is %d and pid requested is %d\n", bsm_tab[i].bs_pid, pid);
        }
        // ensure BS is mapped and starting vaddr is valid for the requested page
        if (bsm_tab[i].bs_status == BSM_MAPPED && (bsm_tab[i].bs_pid == pid) && bsm_tab[i].bs_vpno <= vaddr/NBPG) {
            // valid page given the pid, so let's find where in the backing store the vaddr is
            *pageth = vaddr/NBPG - bsm_tab[i].bs_vpno;
            *store = i;
            restore(ps);
            return(OK);
        }
    }
    // kprintf("Did not find store\n");
    restore(ps);
    return(SYSERR);
}


/*-------------------------------------------------------------------------
 * bsm_map - add an mapping into bsm_tab 
 *-------------------------------------------------------------------------
 */
SYSCALL bsm_map(int pid, int vpno, int source, int npages)
{
    STATWORD ps;
    disable(ps);

    int i = 0;
    if (bsm_tab[source].bs_status == BSM_UNMAPPED && (bsm_tab[source].bs_pid == -1)) {
        // if umapped, we can map this backing store
        bsm_tab[source].bs_status = BSM_MAPPED;
        bsm_tab[source].bs_pid = pid;
        bsm_tab[source].bs_vpno = vpno;
        bsm_tab[source].bs_npages = npages;
        // kprintf("mapped succesfully \n");
        restore(ps);
        return OK;
    }
    // none available
    // kprintf("not mapped\n");
    restore(ps);
    return(SYSERR);
}



/*-------------------------------------------------------------------------
 * bsm_unmap - delete an mapping from bsm_tab
 *-------------------------------------------------------------------------
 */
SYSCALL bsm_unmap(int pid, int vpno, int flag)
{
    STATWORD ps;
    disable(ps);

    int i = 0;
    for (i = 0; i < NUM_BACKING_STORES; i++) {
        if (bsm_tab[i].bs_status == BSM_MAPPED && (bsm_tab[i].bs_pid == pid) && bsm_tab[i].bs_vpno == vpno) {
            // unmap this entry
            bsm_tab[i].bs_status = BSM_UNMAPPED;
            bsm_tab[i].bs_pid = -1;
            // kprintf("unmapped succesfully \n");
            restore(ps);
            return(OK);
        }
    }

    restore(ps);
    return(SYSERR);
}


