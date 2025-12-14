/*
 * Copyright (C) 2026 pdnguyen of HCMC University of Technology VNU-HCM
 */

/* LamiaAtrium release
 * Source Code License Grant: The authors hereby grant to Licensee
 * personal permission to use and modify the Licensed Source Code
 * for the sole purpose of studying while attending the course CO2018.
 */

#include "os-mm.h"
#include "syscall.h"
#include "libmem.h"
#include "queue.h"
#include <stdlib.h>
#include <pthread.h>
#include "common.h"

#ifdef MM64
#include "mm64.h"
#else
#include "mm.h"
#endif

//typedef char BYTE;

// Định nghĩa mutex để bảo vệ các hàng đợi
pthread_mutex_t queue_lock = PTHREAD_MUTEX_INITIALIZER;

int __sys_memmap(struct krnl_t *krnl, uint32_t pid, struct sc_regs* regs)
{
   int memop = regs->a1;
   BYTE value;
   
   /* Find the caller process from kernel's running list using PID */
   struct pcb_t *caller = NULL;

   /*
    * @bksysnet: Please note in the dual spacing design
    *            syscall implementations are in kernel space.
    */

   /* Traverse proclist to find the process with matching PID */
   pthread_mutex_lock(&queue_lock);
   if (krnl->running_list != NULL) {
      struct pcb_t *proc;
      int i;
      int list_size = krnl->running_list->size;

      // Debug: In ra toàn bộ PID trong running_list
      // printf("[DEBUG] running_list PIDs: ");
      // for (int j = 0; j < list_size; j++) {
      //    if (krnl->running_list->proc[j])
      //       printf("%d ", krnl->running_list->proc[j]->pid);
      // }
      // printf("\n");
      
      /* Search through running list for matching PID */
      for (i = 0; i < list_size && caller == NULL; i++) {
         proc = (struct pcb_t *)dequeue(krnl->running_list);
         if (proc != NULL) {
            if (proc->pid == pid) {
               caller = proc;
            }
            /* Re-enqueue process to maintain queue state */
            enqueue(krnl->running_list, proc);
         }
      }
   }
   pthread_mutex_unlock(&queue_lock);
   
   /* Validate caller process was found */
   if (caller == NULL) {
      printf("Error: Process with PID %d not found\n", pid);
      return -1;
   }
	
   switch (memop) {
   case SYSMEM_MAP_OP:
            /* Reserved process case*/
			vmap_pgd_memset(caller, regs->a2, regs->a3);
            break;
   case SYSMEM_INC_OP:
            inc_vma_limit(caller, regs->a2, regs->a3);
            break;
   case SYSMEM_SWP_OP:
            __mm_swap_page(caller, regs->a2, regs->a3);
            break;
   case SYSMEM_IO_READ:
            MEMPHY_read(caller->krnl->mram, regs->a2, &value);
            regs->a3 = value;
            break;
   case SYSMEM_IO_WRITE:
            MEMPHY_write(caller->krnl->mram, regs->a2, regs->a3);
            break;
   default:
            printf("Memop code: %d\n", memop);
            break;
   }
   
   return 0;
}


