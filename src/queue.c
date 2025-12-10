#include <stdio.h>
#include <stdlib.h>
#include "queue.h"

int empty(struct queue_t *q)
{
        if (q == NULL)
                return 1;
        return (q->size == 0);
}

void enqueue(struct queue_t *q, struct pcb_t *proc)
{
        /* TODO: put a new process to queue [q] */
        if (q == NULL) return;
        if (q->size >= MAX_QUEUE_SIZE) {
                return;
        }

        q->proc[q->size] = proc;
        q->size++;
}

struct pcb_t *dequeue(struct queue_t *q)
{
        /* TODO: return a pcb whose prioprity is the highest
         * in the queue [q] and remember to remove it from q
         * */
        if (q == NULL || q->size == 0)
        return NULL;
#ifdef MLQ_SCHED
        struct pcb_t *proc = q->proc[0];

        int j = 0;
        while (j < q->size-1) {
                q->proc[j] = q->proc[j + 1];
                j++;
        }

        q->size--;

        return proc;
#else
        int highest_prio_idx = 0;
        int k = 1;
        while (k<q->size) {
                if (q->proc[k]->prio < q->proc[highest_prio_idx]->prio) {
                        highest_prio_idx = k;
                }
                k++;
        }
        struct pcb_t *proc = q->proc[highest_prio_idx];
        int m = highest_prio_idx;
        while (m < q->size - 1) {
                q->proc[m] = q->proc[m + 1];
                m++;
        }
        q->size--;
        return proc;
#endif
}

struct pcb_t *purgequeue(struct queue_t *q, struct pcb_t *proc)
{
        /* TODO: remove a specific item from queue
         * */
        if (q == NULL || q->size == 0 || proc == NULL)
        return NULL;

        int idx = -1;
        for (int i = 0; i < q->size; i++) {
                if (q->proc[i] == proc) {
                idx = i;
                break;
                }
        }

        if (idx == -1) return NULL;   

        for (int i = idx; i < q->size - 1; i++) {
                q->proc[i] = q->proc[i + 1];
        }

        q->size--;

        return proc;
}
