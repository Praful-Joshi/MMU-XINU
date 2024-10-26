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

  // get the faulting address
  unsigned long addr = read_cr2();
  // either 1) PD doesn't have the desired PT allocated
  // orrrr 2) the desired PT doesn't have the page allocated
  kprintf("Starting page fault handler");
  // check condition 1
  virt_addr_t *virtaddr = (virt_addr_t *) &addr;
  unsigned long pdbr = read_cr3();
  unsigned long pd_offset = virtaddr->pd_offset * 4;
  unsigned long pt_offset = virtaddr->pt_offset * 4;
  unsigned long pg_offset = virtaddr->pg_offset;
  pd_t *pd_entry = (pd_t *) (pdbr + pd_offset);
  if (pd_entry->pd_pres == 0) {
    // PT not present! Need to allocate PT
    int frame;
    get_frm(&frame);
    frm_tab[frame].fr_status = FRM_MAPPED;
    frm_tab[frame].fr_pid = currpid;
    frm_tab[frame].fr_type = FR_TBL;
    frm_tab[frame].fr_vpno = FRAME0+frame;

    pt_t *pt = (pt_t *) (NBPG * (frame + FRAME0));
    pd_entry->pd_base = (((unsigned int) pt) >> 3);
    // page dir entry now valid and present
    pd_entry->pd_pres = 1;
    pd_entry->pd_write = 1;
    int j = 0;
		for (j = 0; j < 1024; j++) {
      // all entries in new page table
			pt[j].pt_pres = 0;
			pt[j].pt_write = 1;
		}

  }
  // else, we know PT present now. Need to allocate page for addr requested
  pt_t *pt_entry = (pt_t *) ((pd_entry->pd_base << 3) + pt_offset);
  if (pt_entry->pt_pres == 0) {
    // get frame and allocate page
    int frame;
    get_frm(&frame);
    frm_tab[frame].fr_status = FRM_MAPPED;
    frm_tab[frame].fr_pid = currpid;
    frm_tab[frame].fr_type = FR_PAGE;
    frm_tab[frame].fr_vpno = FRAME0+frame;
    // page table entry now valid and present
    pt_entry->pt_pres = 1;
    pt_entry->pt_write = 1;

  }

  return OK;
}


