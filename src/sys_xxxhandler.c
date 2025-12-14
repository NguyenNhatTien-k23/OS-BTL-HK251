/*
 * Copyright (C) 2026 pdnguyen of HCMC University of Technology VNU-HCM
 */

/* LamiaAtrium release
 * Source Code License Grant: The authors hereby grant to Licensee
 * personal permission to use and modify the Licensed Source Code
 * for the sole purpose of studying while attending the course CO2018.
 */

#include "common.h"
#include "syscall.h"
#include "stdio.h"
#include "queue.h"
#include <stdlib.h>
#include <pthread.h>

// Khai báo extern để sử dụng queue_lock từ src/sys_mem.c
extern pthread_mutex_t queue_lock;

int __sys_xxxhandler(struct krnl_t *krnl, uint32_t pid, struct sc_regs* regs)
{
    int opcode = regs->a1;
    struct pcb_t *caller = NULL;
    
    pthread_mutex_lock(&queue_lock);
    /* Find the caller process from kernel's running list by PID */
    if (krnl->running_list != NULL) {
        struct pcb_t *proc;
        int i;
        int list_size = krnl->running_list->size;
        
        for (i = 0; i < list_size && caller == NULL; i++) {
            proc = (struct pcb_t *)dequeue(krnl->running_list);
            
            if (proc != NULL) {
                if (proc->pid == pid) {
                    caller = proc;
                }
                enqueue(krnl->running_list, proc);
            }
        }
    }
    pthread_mutex_unlock(&queue_lock);
    
    if (caller == NULL) {
        return -1;
    }
    
    switch (opcode) {
    case 0:
        printf(FORMAT_ARG "\n", regs->a2);
        break;
    case 1:
        printf("The first system call parameter " FORMAT_ARG "\n", regs->a1);
        break;
    case 2:
        printf(FORMAT_ARG "\n", regs->a2 + regs->a3);
        break;
    case 3:
        printf(FORMAT_ARG "\n", regs->a2 * regs->a3);
        break;
    default:
        break;
    }
    
    return 0;
}

