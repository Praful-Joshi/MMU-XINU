/* frame.c - manage physical frames */
#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>

fr_map_t frm_tab[NFRAMES];
q_entry cqueue[NFRAMES];

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

  //We need to do page replacement if no frames are empty
  int frame_id = page_replacement();
  if(frame_id<0){
    restore(ps);
    return SYSERR;
  }

  if(frame_id>=0 || frame_id<NFRAMES){
      free_frm(frame_id);
      frm_tab[frame_id].fr_status = FRM_MAPPED;
      *avail = frame_id;
      restore(ps);
      return OK;
  }

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
  unsigned long pdbr;
  unsigned int vpno;
  unsigned long vaddr;
  unsigned int pd_offset;
  unsigned int pt_offset;
  unsigned int frame_pid;
  int bs_id;
  int pageth;
  pd_t *pd_entry;
  pt_t *pt_entry;
  virt_addr_t *virtual_add;
  if(i<0 || i>=NFRAMES){
    restore(ps);
    return SYSERR;
  }

  if(frm_tab[i].fr_type == FR_PAGE){
    frame_pid = frm_tab[i].fr_pid;
    pdbr = proctab[frame_pid].pdbr;
    bs_id = proctab[frame_pid].store;
    vpno = frm_tab[i].fr_vpno;
    // pageth = vpno - bsm_tab[bs_id].bs_vpno;
    pageth = frm_tab[i].fr_vpno - proctab[frm_tab[i].fr_pid].vhpno;
    vaddr = vpno*4096;
    virtual_add = (virt_addr_t*)&vaddr;
    pd_offset = virtual_add->pd_offset;
    pt_offset = virtual_add->pt_offset;
    pd_entry = (pd_t*)(pdbr + sizeof(pd_t)*pd_offset);
    pt_entry = pd_entry->pd_base*NBPG + sizeof(pt_t)*pt_offset;
    write_bs((i + FRAME0) * NBPG,bs_id,pageth);  
    int tmp_entry = pd_entry->pd_base - FRAME0;
		
    frm_tab[tmp_entry].fr_refcnt--;

		if (frm_tab[tmp_entry].fr_refcnt == 0)
		{
			pd_entry->pd_pres = 0;
			frm_tab[tmp_entry].fr_status = FRM_UNMAPPED;
			frm_tab[tmp_entry].fr_type = FR_PAGE;
			frm_tab[tmp_entry].fr_pid = -1;
			frm_tab[tmp_entry].fr_vpno = 4096;	
		} 	
	
		pt_entry->pt_pres = 0;
    restore(ps);
    return OK;
  }

  restore(ps);
  return SYSERR;
}

int page_replacement(){

  STATWORD ps;
  disable(ps);
  int curr_frame = q_head; //This is basically a frame_id
  int next_frame = -1;
  int prev_frame = -1;
  unsigned long pdbr;
  unsigned int vpno;
  unsigned long vaddr;
  unsigned int pd_offset;
  unsigned int pt_offset;
  pd_t *pd_entry;
  pt_t *pt_entry;
  virt_addr_t *virtual_add;

  if(page_replace_policy == SC){

    while(curr_frame!=-1){
      pdbr = proctab[currpid].pdbr;
      vpno = frm_tab[curr_frame].fr_vpno;
      vaddr = vpno*4096;
      virtual_add = (virt_addr_t*)&vaddr;
      pd_offset = virtual_add->pd_offset;
      pt_offset = virtual_add->pt_offset;
      pd_entry = pdbr + sizeof(pd_t)*pd_offset;
      pt_entry = (pt_t*) (pd_entry->pd_base*NBPG + sizeof(pt_t)*pt_offset);
      //Giving a second chance to the head
      if(pt_entry->pt_acc==1){
        pt_entry->pt_acc = 0;
      }

      else{
        // You need to remove the head
        if(prev_frame==-1){
          // kprintf("Initial head - %d, Next element - %d\n", q_head, cqueue[q_head].next);
          q_head = cqueue[curr_frame].next;
          // kprintf("New head - %d\n", q_head);
          cqueue[curr_frame].next = -1;
          // if(debug_mode==1){
          //   kprintf("166, Replacing frame %d\n", curr_frame);
          // }
          restore(ps);
          return curr_frame;
        }


        else{
          cqueue[prev_frame].next = cqueue[curr_frame].next;
          cqueue[curr_frame].next = -1;
          // if(debug_mode==1){
          //   kprintf("176, Replacing frame %d\n", curr_frame);
          // }
          restore(ps);
          return curr_frame;
        }

        prev_frame = curr_frame;
        curr_frame = cqueue[curr_frame].next;
      }

    }

    curr_frame = q_head;
    // kprintf("%d\n", q_head);
    q_head = cqueue[curr_frame].next;
    cqueue[curr_frame].next = -1;
    //You didn't find anything 
    // if(debug_mode==1){
    //   kprintf("192, Replacing frame %d\n", curr_frame);
    // }
    restore(ps);
    return curr_frame;


  }
}

void init_queue(){
  
  // kprintf("Init queue called\n");
  debug_mode = 1;
  q_head = -1;
  int i;
  for(i=0;i<NFRAMES;i++){
    cqueue[i].fr_id = i;
    cqueue[i].next = -1;
  }
}

void print_queue() {

  int curr = q_head;
  while(curr!=-1){
    kprintf("%d ", curr);
    curr = cqueue[curr].next;
  }
  kprintf("\n");

}

int append_queue(int frame_number){

  STATWORD ps;
  disable(ps);
  // print_queue();
  if(frame_number<0){
    restore(ps);
    return SYSERR;
  }
  int fprev;
  int fcurr;
  if(q_head==-1){
    q_head = frame_number;
    restore(ps);
    return OK;
  }

  fcurr = q_head;
  while(fcurr!=-1){
    fprev = fcurr;
    fcurr = cqueue[fcurr].next;
  }

  cqueue[fprev].next = frame_number;
  cqueue[frame_number].next = -1;
  restore(ps);
  return OK;
}



