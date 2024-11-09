/* frame.c - manage physical frames */
#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>

fr_map_t frm_tab[NFRAMES];

/*-------------------------------------------------------------------------
 * init_frm - initialize frm_tab
 *-------------------------------------------------------------------------
 */
SYSCALL init_frm()
{
  STATWORD ps;
  disable(ps);

  int i = 0;
  for (i = 0; i < NFRAMES; i++) {
    frm_tab[i].fr_status = FRM_UNMAPPED;
    frm_tab[i].fr_pid = -1;
  }

  enable(ps);
  return OK;
}

/*-------------------------------------------------------------------------
 * get_frm - get a free frame according page replacement policy
 *-------------------------------------------------------------------------
 */
SYSCALL get_frm(int* avail)
{
  STATWORD ps;
  disable(ps);

  int i = 0;
  for (i = 0; i < NFRAMES; i++) {
    if (frm_tab[i].fr_status == FRM_UNMAPPED) {
      *avail = i;

      enable(ps);
      return OK;
    }
  }

  // kick out frame


  //none available
  enable(ps);
  return SYSERR;
}

/*-------------------------------------------------------------------------
 * free_frm - free a frame 
 *-------------------------------------------------------------------------
 */
SYSCALL free_frm(int i)
{
  STATWORD ps;
  disable(ps);

  if (i < 0 || i >= NFRAMES) {
    enable(ps);
    return SYSERR;
  }

  if (frm_tab[i].fr_status == FRM_UNMAPPED) {
    enable(ps);
    return OK;
  } else {
    // if mapped, unmap it
    frm_tab[i].fr_status = FRM_UNMAPPED;
    frm_tab[i].fr_pid = -1;
  }


  enable(ps);
  return OK;
}



