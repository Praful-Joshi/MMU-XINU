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
        bsm_tab[i].BS_STATUS = BSM_UNMAPPED;
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
        if (bsm_tab[i].BS_STATUS == BSM_UNMAPPED) {
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
    
    int i = 0;
    // init all backing stores to unmapped
    for (i = 0; i < NUM_BACKING_STORES; i++) {
        bsm_tab[i].BS_STATUS = BSM_UNMAPPED;
    }

    restore(ps);
}

/*-------------------------------------------------------------------------
 * bsm_lookup - lookup bsm_tab and find the corresponding entry
 *-------------------------------------------------------------------------
 */
SYSCALL bsm_lookup(int pid, long vaddr, int* store, int* pageth)
{
    STATWORD ps;
    disable(ps);

    

    restore(ps);
}


/*-------------------------------------------------------------------------
 * bsm_map - add an mapping into bsm_tab 
 *-------------------------------------------------------------------------
 */
SYSCALL bsm_map(int pid, int vpno, int source, int npages)
{
    STATWORD ps;
    disable(ps);

    

    restore(ps);
}



/*-------------------------------------------------------------------------
 * bsm_unmap - delete an mapping from bsm_tab
 *-------------------------------------------------------------------------
 */
SYSCALL bsm_unmap(int pid, int vpno, int flag)
{
    STATWORD ps;
    disable(ps);

    

    restore(ps);
}


