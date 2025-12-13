/*
 * Copyright (C) 2026 pdnguyen of HCMC University of Technology VNU-HCM
 */

 /* LamiaAtrium release
  * Source Code License Grant: The authors hereby grant to Licensee
  * personal permission to use and modify the Licensed Source Code
  * for the sole purpose of studying while attending the course CO2018.
  */

  // #ifdef MM_PAGING
  /*
   * PAGING based Memory Management
   * Virtual memory module mm/mm-vm.c
   */

#include "string.h"
#include "mm64.h"
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

   /*get_vma_by_num - get vm area by numID
    *@mm: memory region
    *@vmaid: ID vm area to alloc memory region
    *
    */
struct vm_area_struct* get_vma_by_num(struct mm_struct* mm, int vmaid) {
  struct vm_area_struct* pvma = mm->mmap;

  if (mm->mmap == NULL)
    return NULL;

  int vmait = pvma->vm_id;

  while (vmait < vmaid) {
    if (pvma == NULL)
      return NULL;

    pvma = pvma->vm_next;
    vmait = pvma->vm_id;
  }

  return pvma;
}

int __mm_swap_page(struct pcb_t* caller, addr_t vicfpn, addr_t swpfpn) {
  __swap_cp_page(caller->krnl->mram, vicfpn, caller->krnl->active_mswp, swpfpn);
  return 0;
}

/*get_vm_area_node - get vm area for a number of pages
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@incpgnum: number of page
 *@vmastart: vma end
 *@vmaend: vma end
 *
 */
struct vm_rg_struct* get_vm_area_node_at_brk(struct pcb_t* caller, int vmaid, addr_t size, addr_t alignedsz) {
  struct vm_rg_struct* newrg;
  /* TODO retrive current vma to obtain newrg, current comment out due to compiler redundant warning*/
  // struct vm_area_struct *cur_vma = get_vma_by_num(caller->kernl->mm, vmaid);

  //newrg = malloc(sizeof(struct vm_rg_struct));

  /* TODO: update the newrg boundary
  // newrg->rg_start = ...
  // newrg->rg_end = ...
  */
  struct vm_area_struct* cur_vma = get_vma_by_num(caller->krnl->mm, vmaid);

  newrg = malloc(sizeof(struct vm_rg_struct));
  newrg->rg_start = cur_vma->sbrk;
  newrg->rg_end = newrg->rg_start + size;
  /* END TODO */

  return newrg;
}

/*validate_overlap_vm_area
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@vmastart: vma end
 *@vmaend: vma end
 *
 */
int validate_overlap_vm_area(struct pcb_t* caller, int vmaid, addr_t vmastart, addr_t vmaend) {
  //struct vm_area_struct *vma = caller->krnl->mm->mmap;

  /* TODO validate the planned memory area is not overlapped */
  if (vmastart >= vmaend) {
    return -1;
  }

  struct vm_area_struct* vma = caller->krnl->mm->mmap;
  if (vma == NULL) {
    return -1;
  }

  /* TODO validate the planned memory area is not overlapped */

  struct vm_area_struct* cur_area = get_vma_by_num(caller->krnl->mm, vmaid);
  if (cur_area == NULL) {
    return -1;
  }

  while (vma != NULL) {
    if (vma != cur_area && OVERLAP(cur_area->vm_start, cur_area->vm_end, vma->vm_start, vma->vm_end)) {
      return -1;
    }
    vma = vma->vm_next;
  }
  /* End TODO*/

  return 0;
}

/*inc_vma_limit - increase vm area limits to reserve space for new variable
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@inc_sz: increment size
 *
 */
int inc_vma_limit(struct pcb_t* caller, int vmaid, addr_t inc_sz) {
  struct vm_rg_struct* newrg = malloc(sizeof(struct vm_rg_struct));

  /* TOTO with new address scheme, the size need tobe aligned
   *      the raw inc_sz maybe not fit pagesize
   */
  addr_t inc_amt = PAGING64_PAGE_ALIGNSZ(inc_sz);

  int incnumpage = inc_amt / PAGING64_PAGESZ;
  struct vm_rg_struct* area = get_vm_area_node_at_brk(caller, vmaid, inc_sz, inc_amt);
  struct vm_area_struct* cur_vma = get_vma_by_num(caller->krnl->mm, vmaid);

  /* TODO Validate overlap of obtained region */
  if (caller == NULL || validate_overlap_vm_area(caller, vmaid, area->rg_start, area->rg_end) < 0)
    return -1; /*Overlap and failed allocation */

  /* TODO: Obtain the new vm area based on vmaid */
  addr_t old_end = cur_vma->vm_end;
  cur_vma->vm_end = area->rg_end;

  //cur_vma->vm_end... 
  // inc_limit_ret...
  /* The obtained vm area (only)
   * now will be alloc real ram region */

  if (vm_map_ram(caller, area->rg_start, area->rg_end,
    old_end, incnumpage, newrg) < 0)
    return -1; /* Map the memory to MEMRAM */

  return 0;
}

int pgalloc(struct pcb_t* proc, uint32_t size, uint32_t reg_index) {

}

int pgfree_data(struct pcb_t* proc, uint32_t reg_index) {

}

int pgread(
  struct pcb_t* proc, // Process executing the instruction
  uint32_t source, // Index of source register
  addr_t offset, // Source address = [source] + [offset]
  uint32_t destination) {
  BYTE data;
  addr_t vadr = proc->regs[source] + offset;
  addr_t pgn = PAGING_PGN(vadr);
  addr_t off = PAGING_OFFST(vadr);

  uint32_t pte = pte_get_entry(proc, pgn);

  if (!(pte & PAGING_PTE_PRESENT_MASK)) {
    if (pte & PAGING_PTE_SWAPPED_MASK) {
      addr_t vicpgn, swpfpn;
      int vicfpn;
      uint32_t vicpte;

      swpfpn = (pte & PAGING_PTE_SWPOFF_MASK) >> PAGING_PTE_SWPOFF_LOBIT;

      if (MEMPHY_get_freefp(proc->krnl->mram, &vicfpn) < 0) {
        struct pgn_t* victim = proc->krnl->mm->fifo_pgn;
        vicpgn = victim->pgn;

        uint32_t vicpte = pte_get_entry(proc, vicpgn);
        int vic_phy_fpn = (vicpte & PAGING_PTE_FPN_MASK) >> PAGING_PTE_FPN_LOBIT;

        int tgtfpn_swp;
        MEMPHY_get_freefp(proc->krnl->active_mswp, &tgtfpn_swp);

        __swap_cp_page(proc->krnl->mram, vic_phy_fpn, proc->krnl->active_mswp, tgtfpn_swp);

        pte_set_swap(proc, vicpgn, 0, tgtfpn_swp);
        vicfpn = vic_phy_fpn;
      }

      __swap_cp_page(proc->krnl->active_mswp, swpfpn, proc->krnl->mram, vicfpn);
      enlist_pgn_node(&proc->krnl->mm->fifo_pgn, pgn);
    }
    else {
      return -1;
    }
  }

  pte = pte_get_entry(proc, pgn);
  addr_t fpn = (pte & PAGING_PTE_FPN_MASK) >> PAGING_PTE_FPN_LOBIT;
  addr_t phyadr = (fpn * PAGING_PAGESZ) + off;

  MEMPHY_read(proc->krnl->mram, phyadr, &data);
  proc->regs[destination] = data;

  return 0;
}

int pgwrite(
  struct pcb_t* proc, // Process executing the instruction
  BYTE data, // Data to be wrttien into memory
  uint32_t destination, // Index of destination register
  addr_t offset) {
  addr_t vadr = proc->regs[destination] + offset;
  addr_t pgn = PAGING_PGN(vadr);
  addr_t off = PAGING_OFFST(vadr);

  uint32_t pte = pte_get_entry(proc, pgn);

  if (!(pte & PAGING_PTE_PRESENT_MASK)) {
    if (pte & PAGING_PTE_SWAPPED_MASK) {
      addr_t vicpgn;
      int vicfpn;
      int swpfpn;

      swpfpn = (pte & PAGING_PTE_SWPOFF_MASK) >> PAGING_PTE_SWPOFF_LOBIT;

      if (MEMPHY_get_freefp(proc->krnl->mram, &vicfpn) < 0) {
        struct pgn_t* victim = proc->krnl->mm->fifo_pgn;
        vicpgn = victim->pgn;

        uint32_t vicpte = pte_get_entry(proc, vicpgn);
        int vic_phy_fpn = (vicpte & PAGING_PTE_FPN_MASK) >> PAGING_PTE_FPN_LOBIT;

        int tgtfpn_swp;

        MEMPHY_get_freefp(proc->krnl->active_mswp, &tgtfpn_swp);

        __swap_cp_page(proc->krnl->mram, vic_phy_fpn, proc->krnl->active_mswp, tgtfpn_swp);
        pte_set_swap(proc, vicpgn, 0, tgtfpn_swp);
        vicfpn = vic_phy_fpn;
      }

      __swap_cp_page(proc->krnl->active_mswp, swpfpn, proc->krnl->mram, vicfpn);
      pte_set_fpn(proc, pgn, vicfpn);


      enlist_pgn_node(&proc->krnl->mm->fifo_pgn, pgn);
    }
    else {
      return -1;
    }
  }

  pte = pte_get_entry(proc, pgn);
  addr_t fpn = (pte & PAGING_PTE_FPN_MASK) >> PAGING_PTE_FPN_LOBIT;
  addr_t phyadr = (fpn * PAGING_PAGESZ) + off;

  MEMPHY_write(proc->krnl->mram, phyadr, data);

  return 0;
}

// #endif