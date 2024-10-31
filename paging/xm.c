/* xm.c = xmmap xmunmap */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>


/*-------------------------------------------------------------------------
 * xmmap - xmmap
 *-------------------------------------------------------------------------
 */
SYSCALL xmmap(int virtpage, bsd_t source, int npages)
{
  STATWORD ps;
  disable(ps);
  if (npages > 256) {
    enable(ps);
    return SYSERR;
  }
  if (source < 0 || source >= NUM_BACKING_STORES) {
    enable(ps);
    return SYSERR;
  }
  if (virtpage < 4096) {
    enable(ps);
    return SYSERR;
  }
  // reserve
  bsm_map(currpid, virtpage, source, npages);
  // add values to proctab mapping
  // kprintf("PID %d is using vmem\n", currpid);
  proctab[currpid].using_vmem = 0;
  // these technically are used for vheap, but we'll hack it to make it work for vmmap
  proctab[currpid].store = source;
  proctab[currpid].vhpno = virtpage;
  proctab[currpid].vhpnpages = npages;
  // kprintf("Backing store %d mapped to process %d at virtpage %d\n", source, bsm_tab[source].bs_pid, virtpage);
  enable(ps);
  return OK;
}



/*-------------------------------------------------------------------------
 * xmunmap - xmunmap
 *-------------------------------------------------------------------------
 */
SYSCALL xmunmap(int virtpage)
{
  STATWORD ps;
  disable(ps);
  // get the store 
  int store = proctab[currpid].store;
  if (bsm_tab[store].bs_vpno != virtpage) {
    return SYSERR;
  } else {
    // only unmap if no other proc is using the backing store
    int i = 0;
    // proc no longer uses vmem
    proctab[currpid].using_vmem = -1;
    proctab[currpid].store = -1;
    for (i = 0; i < NPROC; i++) {
      if (i != currpid && proctab[i].using_vmem == 0 && proctab[i].store == store ) {
        bsm_tab[store].bs_pid = i;
        bsm_tab[store].bs_vpno = proctab[i].vhpno;
        enable(ps);
        return OK;
      }
    }
    // else, unmap since nobody else is using
    bsm_unmap(currpid, virtpage, 0);
    
    enable(ps);
    return OK;
  }
  enable(ps);
  return SYSERR;
}
