/* pfint.c - pfint */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>


/*-------------------------------------------------------------------------
 * pfint - paging fault ISR
 *-------------------------------------------------------------------------
 */
SYSCALL pfint()
{
  STATWORD ps;
  disable(ps);

  // get the faulting address
  unsigned long addr = read_cr2();
  // either 1) PD doesn't have the desired PT allocated
  // orrrr 2) the desired PT doesn't have the page allocated
  // kprintf("Starting page fault handler for pid %d\n", currpid);
  // check condition 1
  virt_addr_t *virtaddr = (virt_addr_t *) &addr;
  unsigned long pdbr = proctab[currpid].pdbr;
  unsigned long pd_offset = virtaddr->pd_offset * 4;
  unsigned long pt_offset = virtaddr->pt_offset * 4;
  unsigned long pg_offset = virtaddr->pg_offset;
  pd_t *pd_entry = (pd_t *) (pdbr + pd_offset);
  if (pd_entry->pd_pres == 0) {
    // PT not present! Need to allocate PT
    // kprintf("Allocating PT \n");
    int frame;
    get_frm(&frame);
    frm_tab[frame].fr_status = FRM_MAPPED;
    frm_tab[frame].fr_pid = currpid;
    frm_tab[frame].fr_type = FR_TBL;
    frm_tab[frame].fr_vpno = FRAME0+frame;

    pt_t *pt = (pt_t *) (NBPG * (frame + FRAME0));
    pd_entry->pd_base = FRAME0 + frame;
    // page dir entry now valid and present
    pd_entry->pd_pres = 1;
    pd_entry->pd_write = 1;
    int j = 0;
		for (j = 0; j < 1024; j++) {
      // all entries in new page table
			pt[j].pt_pres = 0;
			pt[j].pt_write = 1;
      pt[j].pt_user = 0;
			pt[j].pt_pwt = 0;
			pt[j].pt_pcd = 0;
			pt[j].pt_acc = 0;
			pt[j].pt_dirty = 0;
			pt[j].pt_mbz = 0;
			pt[j].pt_global = 0;
			pt[j].pt_avail = 0;
			pt[j].pt_base = 0;
		}

  }
  // else, we know PT present now. Need to allocate page for addr requested
  pt_t *pt_entry = (pt_t *) ((pd_entry->pd_base * NBPG) + (pt_offset));
  if (pt_entry->pt_pres == 0) {
    // get frame and allocate page
    int frame;
    get_frm(&frame);
    frm_tab[frame].fr_status = FRM_MAPPED;
    frm_tab[frame].fr_pid = currpid;
    frm_tab[frame].fr_type = FR_PAGE;
    frm_tab[frame].fr_vpno = addr / NBPG;
    // page table entry now valid and present
    pt_entry->pt_pres = 1;
    pt_entry->pt_write = 1;
    frm_tab[frame].fr_dirty = 1;
    pt_entry->pt_base = FRAME0 + frame;
    // if accessing VM, then it has to be in the backing store. Find out where (assuming vmmap only now)
    int store = -1;
    int pageth = -1;
    bsm_lookup(currpid, addr, &store, &pageth);
    // kprintf("Store: %d\n", store);
    // kprintf("Page offset: %d\n", pageth);
    // kprintf("Current pid: %d\n", currpid);
    // kprintf("Frame for page: %d\n", frame);
    read_bs((FRAME0+frame)*NBPG, store, pageth);
    // kprintf("Got %c from backing store in proc %d\n", *(char *) ((FRAME0+frame)*NBPG), currpid);

    // if curr proc is using xmmap/xmunmap, then invalidate other procs relevant pages
    int id = 0;
    int vpage = addr / NBPG;
    int pageoff = vpage - proctab[currpid].vhpno;

    for (id = 0; id < NPROC; id++) {
      // kprintf(" id %d\n", id);
      if (id == currpid) {
        // kprintf("Skipping id %d\n", id);
        continue;
      }
      struct pentry *cur = &proctab[id];
      // make sure the page is accessible
      
      if (cur->using_vmem == 0 && cur->store == store && cur->vhpnpages >= pageoff) {
        // then we invalidate the appropriate page table entry for the proc relavant page
        // kprintf("Invalidating proc %d pages\n", id);
        int vpage_cur = cur->vhpno + pageoff; // the vpage we gotta invalidate
        virt_addr_t *addr_curr = vpage_cur * NBPG;
        pd_t *pd_entry_cur = (pd_t *) (cur->pdbr + (addr_curr->pd_offset * 4));
        pt_t *pt_entry_cur = (pt_t *) ((pd_entry_cur->pd_base * NBPG) + (addr_curr->pt_offset * 4));
        pt_entry_cur->pt_pres = 0; // invalidate
        
        // now invalidate the frame
        int f = 0;
        for (f = 0; f < NFRAMES; f++) {
          if (frm_tab[f].fr_pid == id && frm_tab[f].fr_vpno == vpage_cur) {
            // free frame
            free_frm(f);
            // kprintf("Invalidating frame with page %d for proc %d, current proc %d\n", vpage_cur, id, currpid);
            break;
          }
        }
      }
    }

  }
  enable(ps);
  return OK;
}


