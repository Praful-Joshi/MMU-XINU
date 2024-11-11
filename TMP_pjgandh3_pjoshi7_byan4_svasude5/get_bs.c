#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>

int get_bs(bsd_t bs_id, unsigned int npages) {

  /* requests a new mapping of npages with ID map_id */
    if (bs_id < 0 || bs_id >= NUM_BACKING_STORES || npages > 256) {
      return SYSERR;
    }
    if (bsm_tab[bs_id].bs_status == BSM_UNMAPPED && bsm_tab[bs_id].bs_pid == -1) {
      return npages;
    } else if (bsm_tab[bs_id].bs_status == BSM_MAPPED && bsm_tab[bs_id].bs_pid == currpid && bsm_tab[bs_id].bs_npages >= npages) {
      return bsm_tab[bs_id].bs_npages;
    }

    return SYSERR;
    
}


