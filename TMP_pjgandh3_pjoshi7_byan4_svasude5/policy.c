/* policy.c = srpolicy*/

#include <conf.h>
#include <kernel.h>
#include <paging.h>

int debug_mode;
int q_head;
int page_replace_policy;
/*-------------------------------------------------------------------------
 * srpolicy - set page replace policy 
 *-------------------------------------------------------------------------
 */
SYSCALL srpolicy(int policy)
{
  /* sanity check ! */

  page_replace_policy = policy;
  debug_mode = 1;
  return OK;
}

/*-------------------------------------------------------------------------
 * grpolicy - get page replace policy 
 *-------------------------------------------------------------------------
 */
SYSCALL grpolicy()
{
  return page_replace_policy;
}