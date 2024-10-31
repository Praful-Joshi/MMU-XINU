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
  if (npages > 256) {
    return SYSERR;
  }
  if (source < 0 || source >= NUM_BACKING_STORES) {
    return SYSERR;
  }
  if (virtpage < 4096) {
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
  return OK;
}



/*-------------------------------------------------------------------------
 * xmunmap - xmunmap
 *-------------------------------------------------------------------------
 */
SYSCALL xmunmap(int virtpage)
{
  kprintf("To be implemented!");
  return SYSERR;
}
