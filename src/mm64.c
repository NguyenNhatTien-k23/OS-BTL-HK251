/*
 * Copyright (C) 2026 pdnguyen of HCMC University of Technology VNU-HCM
 */

 /* LamiaAtrium release
  * Source Code License Grant: The authors hereby grant to Licensee
  * personal permission to use and modify the Licensed Source Code
  * for the sole purpose of studying while attending the course CO2018.
  */

  /*
   * PAGING based Memory Management
   * Memory management unit mm/mm.c
   */

#include "mm64.h"
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>

#define DEBUG

static pthread_mutex_t mm64_lock = PTHREAD_MUTEX_INITIALIZER;

#if defined(MM64)

   /*
    * init_pte - Initialize PTE entry
    */
int init_pte(addr_t* pte,
  int pre,    // present
  addr_t fpn,    // FPN
  int drt,    // dirty
  int swp,    // swap
  int swptyp, // swap type
  addr_t swpoff) // swap offset
{
  if (pre != 0) {
    if (swp == 0) { // Non swap ~ page online
      if (fpn == 0)
        return -1;  // Invalid setting

      /* Valid setting with FPN */
      SETBIT(*pte, PAGING_PTE_PRESENT_MASK);
      CLRBIT(*pte, PAGING_PTE_SWAPPED_MASK);
      CLRBIT(*pte, PAGING_PTE_DIRTY_MASK);

      SETVAL(*pte, fpn, PAGING_PTE_FPN_MASK, PAGING_PTE_FPN_LOBIT);
    }
    else { // page swapped
      SETBIT(*pte, PAGING_PTE_PRESENT_MASK);
      SETBIT(*pte, PAGING_PTE_SWAPPED_MASK);
      CLRBIT(*pte, PAGING_PTE_DIRTY_MASK);

      SETVAL(*pte, swptyp, PAGING_PTE_SWPTYP_MASK, PAGING_PTE_SWPTYP_LOBIT);
      SETVAL(*pte, swpoff, PAGING_PTE_SWPOFF_MASK, PAGING_PTE_SWPOFF_LOBIT);
    }
  }

  return 0;
}


/*
 * get_pd_from_pagenum - Parse address to 5 page directory level
 * @pgn   : pagenumer
 * @pgd   : page global directory
 * @p4d   : page level directory
 * @pud   : page upper directory
 * @pmd   : page middle directory
 * @pt    : page table
 */
int get_pd_from_address(addr_t addr, addr_t* pgd, addr_t* p4d, addr_t* pud, addr_t* pmd, addr_t* pt) {
  // 	/* Extract page direactories */
  *pgd = (addr & PAGING64_ADDR_PGD_MASK) >> PAGING64_ADDR_PGD_LOBIT;
  *p4d = (addr & PAGING64_ADDR_P4D_MASK) >> PAGING64_ADDR_P4D_LOBIT;
  *pud = (addr & PAGING64_ADDR_PUD_MASK) >> PAGING64_ADDR_PUD_LOBIT;
  *pmd = (addr & PAGING64_ADDR_PMD_MASK) >> PAGING64_ADDR_PMD_LOBIT;
  *pt = (addr & PAGING64_ADDR_PT_MASK) >> PAGING64_ADDR_PT_LOBIT;

  /* TODO: implement the page direactories mapping */
  //So what do i map this to what?
  //What table?
  // if (pgd) *pgd = PAGING64_ADDR_PGD(addr);
  // if (p4d) *p4d = PAGING64_ADDR_P4D(addr);
  // if (pud) *pud = PAGING64_ADDR_PUD(addr);
  // if (pmd) *pmd = PAGING64_ADDR_PMD(addr);
  // if (pt) *pt = PAGING64_ADDR_PT(addr);

// fprintf(stderr, "Get PD from addr: 0x%llx => pgd: 0x%llx p4d: 0x%llx pud: 0x%llx pmd: 0x%llx pt: 0x%llx\n",
//   addr, (pgd)?*pgd:0, (p4d)?*p4d:0, (pud)?*pud:0, (pmd)?*pmd:0, (pt)?*pt:0);

  return 0;
}

/*
 * get_pd_from_pagenum - Parse page number to 5 page directory level
 * @pgn   : pagenumer
 * @pgd   : page global directory
 * @p4d   : page level directory
 * @pud   : page upper directory
 * @pmd   : page middle directory
 * @pt    : page table
 */
int get_pd_from_pagenum(addr_t pgn, addr_t* pgd, addr_t* p4d, addr_t* pud, addr_t* pmd, addr_t* pt) {
  /* Shift the address to get page num and perform the mapping*/
  return get_pd_from_address(pgn << PAGING64_ADDR_PT_SHIFT,
    pgd, p4d, pud, pmd, pt);
}


/*
 * pte_set_swap - Set PTE entry for swapped page
 * @pte    : target page table entry (PTE)
 * @swptyp : swap type
 * @swpoff : swap offset
 */
int pte_set_swap(struct pcb_t* caller, addr_t pgn, int swptyp, addr_t swpoff) {
  pthread_mutex_lock(&mm64_lock);

  struct krnl_t* krnl = caller->krnl;
  struct mm_struct *mm = krnl->mm;
  addr_t* pte;
  addr_t pgd = 0;
  addr_t p4d = 0;
  addr_t pud = 0;
  addr_t pmd = 0;
  addr_t pt = 0;

  // dummy pte alloc to avoid runtime error
  pte = malloc(sizeof(addr_t));

#ifdef MM64	
  /* Get value from the system */
  /* TODO Perform multi-level page mapping */
  get_pd_from_pagenum(pgn, &pgd, &p4d, &pud, &pmd, &pt);

  /* Level 1: PGD */
  if (krnl->mm->pgd[pgd] == 0) {
    krnl->mm->pgd[pgd] = (addr_t)calloc(PAGING64_MAX_PGN, sizeof(addr_t));
  }
  addr_t* p4d_tbl = (addr_t*)krnl->mm->pgd[pgd];

  /* Level 2: P4D */
  if (p4d_tbl[p4d] == 0) {
    p4d_tbl[p4d] = (addr_t)calloc(PAGING64_MAX_PGN, sizeof(addr_t));
  }
  addr_t* pud_tbl = (addr_t*)p4d_tbl[p4d];

  /* Level 3: PUD */
  if (pud_tbl[pud] == 0) {
    pud_tbl[pud] = (addr_t)calloc(PAGING64_MAX_PGN, sizeof(addr_t));
  }
  addr_t* pmd_tbl = (addr_t*)pud_tbl[pud];

  /* Level 4: PMD */
  if (pmd_tbl[pmd] == 0) {
    pmd_tbl[pmd] = (addr_t)calloc(PAGING64_MAX_PGN, sizeof(addr_t));
  }
  addr_t* pt_tbl = (addr_t*)pmd_tbl[pmd];

  /* Level 5: PT */
  pte = &pt_tbl[pt];

#else
  pte = &krnl->mm->pgd[pgn];
#endif

  SETBIT(*pte, PAGING_PTE_PRESENT_MASK);
  SETBIT(*pte, PAGING_PTE_SWAPPED_MASK);

  SETVAL(*pte, swptyp, PAGING_PTE_SWPTYP_MASK, PAGING_PTE_SWPTYP_LOBIT);
  SETVAL(*pte, swpoff, PAGING_PTE_SWPOFF_MASK, PAGING_PTE_SWPOFF_LOBIT);

  pthread_mutex_unlock(&mm64_lock);
  return 0;
}

/*
 * pte_set_fpn - Set PTE entry for on-line page
 * @pte   : target page table entry (PTE)
 * @fpn   : frame page number (FPN)
 */
int pte_set_fpn(struct pcb_t* caller, addr_t pgn, addr_t fpn) {
  pthread_mutex_lock(&mm64_lock);

  struct krnl_t* krnl = caller->krnl;
  struct mm_struct *mm = krnl->mm;

  addr_t* pte;
  addr_t pgd = 0;
  addr_t p4d = 0;
  addr_t pud = 0;
  addr_t pmd = 0;
  addr_t pt = 0;

  // dummy pte alloc to avoid runtime error
  pte = malloc(sizeof(addr_t));

#ifdef MM64	
  /* Get value from the system */
  /* TODO Perform multi-level page mapping */
  get_pd_from_pagenum(pgn, &pgd, &p4d, &pud, &pmd, &pt);

  /* Level 1: PGD */
  if (krnl->mm->pgd[pgd] == 0) {
    krnl->mm->pgd[pgd] = (addr_t)calloc(PAGING64_MAX_PGN, sizeof(addr_t));
  }
  addr_t* p4d_tbl = (addr_t*)krnl->mm->pgd[pgd];

  /* Level 2: P4D */
  if (p4d_tbl[p4d] == 0) {
    p4d_tbl[p4d] = (addr_t)calloc(PAGING64_MAX_PGN, sizeof(addr_t));
  }
  addr_t* pud_tbl = (addr_t*)p4d_tbl[p4d];

  /* Level 3: PUD */
  if (pud_tbl[pud] == 0) {
    pud_tbl[pud] = (addr_t)calloc(PAGING64_MAX_PGN, sizeof(addr_t));
  }
  addr_t* pmd_tbl = (addr_t*)pud_tbl[pud];

  /* Level 4: PMD */
  if (pmd_tbl[pmd] == 0) {
    pmd_tbl[pmd] = (addr_t)calloc(PAGING64_MAX_PGN, sizeof(addr_t));
  }
  addr_t* pt_tbl = (addr_t*)pmd_tbl[pmd];

  /* Level 5: PT */
  pte = &pt_tbl[pt];


#else
  pte = &krnl->mm->pgd[pgn];
#endif

  SETBIT(*pte, PAGING_PTE_PRESENT_MASK);
  CLRBIT(*pte, PAGING_PTE_SWAPPED_MASK);

  SETVAL(*pte, fpn, PAGING_PTE_FPN_MASK, PAGING_PTE_FPN_LOBIT);

  pthread_mutex_unlock(&mm64_lock);
  return 0;
}


/* Get PTE page table entry
 * @caller : caller
 * @pgn    : page number
 * @ret    : page table entry
 **/
uint32_t pte_get_entry(struct pcb_t* caller, addr_t pgn) {
  pthread_mutex_lock(&mm64_lock);

  if (caller == NULL || caller->krnl == NULL || caller->krnl->mm == NULL) {
    pthread_mutex_unlock(&mm64_lock);
    return 0;
  }

  struct krnl_t* krnl = caller->krnl;
  struct mm_struct *mm = krnl->mm;

  if (mm->pgd == NULL || mm->p4d == NULL || mm->pud == NULL || mm->pmd == NULL || mm->pt == NULL) {
    pthread_mutex_unlock(&mm64_lock);
    return 0;
  }

  uint32_t pte = 0;
  addr_t pgd = 0;
  addr_t p4d = 0;
  addr_t pud = 0;
  addr_t pmd = 0;
  addr_t	 pt = 0;

  /* TODO Perform multi-level page mapping */
  get_pd_from_pagenum(pgn, &pgd, &p4d, &pud, &pmd, &pt);

  /* Level 1: PGD */
  if (krnl->mm->pgd[pgd] == 0) {
    krnl->mm->pgd[pgd] = (addr_t)calloc(PAGING64_MAX_PGN, sizeof(addr_t));
  }
  addr_t* p4d_tbl = (addr_t*)krnl->mm->pgd[pgd];

  /* Level 2: P4D */
  if (p4d_tbl[p4d] == 0) {
    p4d_tbl[p4d] = (addr_t)calloc(PAGING64_MAX_PGN, sizeof(addr_t));
  }
  addr_t* pud_tbl = (addr_t*)p4d_tbl[p4d];

  /* Level 3: PUD */
  if (pud_tbl[pud] == 0) {
    pud_tbl[pud] = (addr_t)calloc(PAGING64_MAX_PGN, sizeof(addr_t));
  }
  addr_t* pmd_tbl = (addr_t*)pud_tbl[pud];

  /* Level 4: PMD */
  if (pmd_tbl[pmd] == 0) {
    pmd_tbl[pmd] = (addr_t)calloc(PAGING64_MAX_PGN, sizeof(addr_t));
  }
  addr_t* pt_tbl = (addr_t*)pmd_tbl[pmd];

  /* Level 5: PT */
  pte = (uint32_t)pt_tbl[pt];

  pthread_mutex_unlock(&mm64_lock);
  return pte;
}

/* Set PTE page table entry
 * @caller : caller
 * @pgn    : page number
 * @ret    : page table entry
 **/
int pte_set_entry(struct pcb_t* caller, addr_t pgn, uint32_t pte_val) {
  pthread_mutex_lock(&mm64_lock);

  struct krnl_t *krnl = caller->krnl;

  uint32_t pte = 0;
  addr_t pgd = 0;
  addr_t p4d = 0;
  addr_t pud = 0;
  addr_t pmd = 0;
  addr_t	 pt = 0;

  /* TODO Perform multi-level page mapping */
  get_pd_from_pagenum(pgn, &pgd, &p4d, &pud, &pmd, &pt);

  /* Level 1: PGD */
  if (krnl->mm->pgd[pgd] == 0) {
    krnl->mm->pgd[pgd] = (addr_t)calloc(PAGING64_MAX_PGN, sizeof(addr_t));
  }
  addr_t* p4d_tbl = (addr_t*)krnl->mm->pgd[pgd];

  /* Level 2: P4D */
  if (p4d_tbl[p4d] == 0) {
    p4d_tbl[p4d] = (addr_t)calloc(PAGING64_MAX_PGN, sizeof(addr_t));
  }
  addr_t* pud_tbl = (addr_t*)p4d_tbl[p4d];

  /* Level 3: PUD */
  if (pud_tbl[pud] == 0) {
    pud_tbl[pud] = (addr_t)calloc(PAGING64_MAX_PGN, sizeof(addr_t));
  }
  addr_t* pmd_tbl = (addr_t*)pud_tbl[pud];

  /* Level 4: PMD */
  if (pmd_tbl[pmd] == 0) {
    pmd_tbl[pmd] = (addr_t)calloc(PAGING64_MAX_PGN, sizeof(addr_t));
  }
  addr_t* pt_tbl = (addr_t*)pmd_tbl[pmd];

  /* Level 5: PT */
  pt_tbl[pt] = pte_val;

  pthread_mutex_unlock(&mm64_lock);
  return 0;
}


/*
 * vmap_pgd_memset - map a range of page at aligned address
 */
int vmap_pgd_memset(struct pcb_t* caller,           // process call
  addr_t addr,                       // start address which is aligned to pagesz
  int pgnum)                      // num of mapping page
{
  int pgit = 0;
  uint64_t pattern = 0xdeadbeef;
  addr_t pgn;

  struct krnl_t *krnl = caller->krnl;

  /* TODO memset the page table with given pattern
   */
  for (pgit = 0; pgit < pgnum; ++pgit) {
  #ifdef MM64
    pgn = (addr / PAGING64_PAGESZ) + pgit;
  #else
    pgn = (addr / PAGING_PAGESZ) + pgit;
  #endif
    addr_t pgd, p4d, pud, pmd, pt;
    get_pd_from_pagenum(pgn, &pgd, &p4d, &pud, &pmd, &pt);

    /* Level 1: PGD */
    if (krnl->mm->pgd[pgd] == 0) {
      krnl->mm->pgd[pgd] = (addr_t)calloc(PAGING64_MAX_PGN, sizeof(addr_t));
    }
    addr_t* p4d_tbl = (addr_t*)krnl->mm->pgd[pgd];

    /* Level 2: P4D */
    if (p4d_tbl[p4d] == 0) {
      p4d_tbl[p4d] = (addr_t)calloc(PAGING64_MAX_PGN, sizeof(addr_t));
    }
    addr_t* pud_tbl = (addr_t*)p4d_tbl[p4d];

    /* Level 3: PUD */
    if (pud_tbl[pud] == 0) {
      pud_tbl[pud] = (addr_t)calloc(PAGING64_MAX_PGN, sizeof(addr_t));
    }
    addr_t* pmd_tbl = (addr_t*)pud_tbl[pud];

    /* Level 4: PMD */
    if (pmd_tbl[pmd] == 0) {
      pmd_tbl[pmd] = (addr_t)calloc(PAGING64_MAX_PGN, sizeof(addr_t));
    }
    
    krnl->mm->pt[pt] = pattern;
  }

  return 0;
}

/*
 * vmap_page_range - map a range of page at aligned address
 */
addr_t vmap_page_range(struct pcb_t* caller,           // process call
  addr_t addr,                       // start address which is aligned to pagesz
  int pgnum,                      // num of mapping page
  struct framephy_struct* frames, // list of the mapped frames
  struct vm_rg_struct* ret_rg)    // return mapped region, the real mapped fp
{                                                   // no guarantee all given pages are mapped
  struct framephy_struct* fpit = frames;
  int pgit = 0;
  addr_t pgn64;

  /* TODO: update the rg_end and rg_start of ret_rg
  //ret_rg->rg_end =  ....
  //ret_rg->rg_start = ...
  //ret_rg->vmaid = ...
  */
  ret_rg->rg_start = addr;
  ret_rg->rg_end = addr;

  /* TODO map range of frame to address space
   *      [addr to addr + pgnum*PAGING_PAGESZ
   *      in page table caller->krnl->mm->pgd,
   *                    caller->krnl->mm->pud...
   *                    ...
   */

  for (pgit = 0; pgit < pgnum && fpit != NULL; ++pgit) {

    addr_t cur_addr = addr + (addr_t)pgit * PAGING64_PAGESZ;
    pgn64 = cur_addr >> PAGING64_ADDR_PT_SHIFT;

    pte_set_fpn(caller, pgn64, fpit->fpn);

    fpit = fpit->fp_next;
    ret_rg->rg_end += PAGING64_PAGESZ;

    /* Tracking for later page replacement activities (if needed)
    * Enqueue new usage page */
    enlist_pgn_node(&caller->krnl->mm->fifo_pgn, pgn64);
  }

  return 0;
}

/*
 * alloc_pages_range - allocate req_pgnum of frame in ram
 * @caller    : caller
 * @req_pgnum : request page num
 * @frm_lst   : frame list
 */

addr_t alloc_pages_range(struct pcb_t* caller, int req_pgnum, struct framephy_struct** frm_lst) {
  if (req_pgnum <= 0)
    return 0;

  addr_t fpn;
  struct framephy_struct* head = NULL;
  struct framephy_struct* tail = NULL;

  for (int i = 0; i < req_pgnum; i++) {

    /* Try to allocate a free frame */
    if (MEMPHY_get_freefp(caller->krnl->mram, &fpn) != 0) {
      /* Roll back: free all previously allocated frames */
      struct framephy_struct* cur = head;
      while (cur) {
        MEMPHY_put_freefp(caller->krnl->mram, cur->fpn);
        struct framephy_struct* tmp = cur;
        cur = cur->fp_next;
        free(tmp);
      }
      return -1;
    }

    /* Create a new node for this successfully allocated frame */
    struct framephy_struct* node = malloc(sizeof(struct framephy_struct));
    node->fpn = fpn;
    node->fp_next = NULL;

    if (!head) {
      head = node;
      tail = node;
    }
    else {
      tail->fp_next = node;
      tail = node;
    }
  }

  *frm_lst = head;
  return 0;
}

/*
 * vm_map_ram - do the mapping all vm are to ram storage device
 * @caller    : caller
 * @astart    : vm area start
 * @aend      : vm area end
 * @mapstart  : start mapping point
 * @incpgnum  : number of mapped page
 * @ret_rg    : returned region
 */
addr_t vm_map_ram(struct pcb_t* caller, addr_t astart, addr_t aend, addr_t mapstart, int incpgnum, struct vm_rg_struct* ret_rg) {
  struct framephy_struct* frm_lst = NULL;
  addr_t ret_alloc = 0;
  //  int pgnum = incpgnum;

    /*@bksysnet: author provides a feasible solution of getting frames
     *FATAL logic in here, wrong behaviour if we have not enough page
     *i.e. we request 1000 frames meanwhile our RAM has size of 3 frames
     *Don't try to perform that case in this simple work, it will result
     *in endless procedure of swap-off to get frame and we have not provide
     *duplicate control mechanism, keep it simple
     */
     // ret_alloc = alloc_pages_range(caller, pgnum, &frm_lst);

  if (ret_alloc < 0 && ret_alloc != -3000)
    return -1;

  /* Out of memory */
  if (ret_alloc == -3000) {
    return -1;
  }

  /* it leaves the case of memory is enough but half in ram, half in swap
   * do the swaping all to swapper to get the all in ram */
  vmap_page_range(caller, mapstart, incpgnum, frm_lst, ret_rg);

  return 0;
}

/* Swap copy content page from source frame to destination frame
 * @mpsrc  : source memphy
 * @srcfpn : source physical page number (FPN)
 * @mpdst  : destination memphy
 * @dstfpn : destination physical page number (FPN)
 **/
int __swap_cp_page(struct memphy_struct* mpsrc, addr_t srcfpn,
  struct memphy_struct* mpdst, addr_t dstfpn) {
  int cellidx;
  addr_t addrsrc, addrdst;
  for (cellidx = 0; cellidx < PAGING_PAGESZ; cellidx++) {
    addrsrc = srcfpn * PAGING_PAGESZ + cellidx;
    addrdst = dstfpn * PAGING_PAGESZ + cellidx;

    BYTE data;
    MEMPHY_read(mpsrc, addrsrc, &data);
    MEMPHY_write(mpdst, addrdst, data);
  }

  return 0;
}

/*
 *Initialize a empty Memory Management instance
 * @mm:     self mm
 * @caller: mm owner
 */
int init_mm(struct mm_struct* mm, struct pcb_t* caller) {
  struct vm_area_struct* vma0 = malloc(sizeof(struct vm_area_struct));

  /* TODO init page table directory */
  if (!mm) {
    printf("init_mm: mm is NULL\n");
    return -1;
  }

  if (!caller) {
    printf("init_mm: caller is NULL\n");
    return -1;
  }

  mm->pgd = calloc(PAGING64_MAX_PGN, sizeof(addr_t));
  mm->p4d = calloc(PAGING64_MAX_PGN, sizeof(addr_t));
  mm->pud = calloc(PAGING64_MAX_PGN, sizeof(addr_t));
  mm->pmd = calloc(PAGING64_MAX_PGN, sizeof(addr_t));
  mm->pt  = NULL;

  /* By default the owner comes with at least one vma */
  vma0->vm_id = 0;
  vma0->vm_start = 0;
  vma0->vm_end = 0;
  vma0->sbrk = vma0->vm_start;
  struct vm_rg_struct* first_rg = init_vm_rg(vma0->vm_start, vma0->vm_end);
  enlist_vm_rg_node(&vma0->vm_freerg_list, first_rg);

  /* TODO update VMA0 next */
  vma0->vm_next = NULL;

  /* Point vma owner backward */
  vma0->vm_mm = mm;

  /* TODO: update mmap */
  mm->mmap = vma0;
  memset(mm->symrgtbl, 0, sizeof(mm->symrgtbl));
  mm->fifo_pgn = NULL;

  caller->krnl->mm = mm;

  return 0;
}

struct vm_rg_struct* init_vm_rg(addr_t rg_start, addr_t rg_end) {
  struct vm_rg_struct* rgnode = malloc(sizeof(struct vm_rg_struct));

  rgnode->rg_start = rg_start;
  rgnode->rg_end = rg_end;
  rgnode->rg_next = NULL;

  return rgnode;
}

int enlist_vm_rg_node(struct vm_rg_struct** rglist, struct vm_rg_struct* rgnode) {
  rgnode->rg_next = *rglist;
  *rglist = rgnode;

  return 0;
}

int enlist_pgn_node(struct pgn_t** plist, addr_t pgn) {
  struct pgn_t* pnode = malloc(sizeof(struct pgn_t));

  pnode->pgn = pgn;
  pnode->pg_next = *plist;
  *plist = pnode;

  return 0;
}

int print_list_fp(struct framephy_struct* ifp) {
  struct framephy_struct* fp = ifp;

  printf("print_list_fp: ");
  if (fp == NULL) { printf("NULL list\n"); return -1; }
  printf("\n");
  while (fp != NULL) {
    printf("fp[" FORMAT_ADDR "]\n", fp->fpn);
    fp = fp->fp_next;
  }
  printf("\n");
  return 0;
}

int print_list_rg(struct vm_rg_struct* irg) {
  struct vm_rg_struct* rg = irg;

  printf("print_list_rg: ");
  if (rg == NULL) { printf("NULL list\n"); return -1; }
  printf("\n");
  while (rg != NULL) {
    printf("rg[" FORMAT_ADDR "->"  FORMAT_ADDR "]\n", rg->rg_start, rg->rg_end);
    rg = rg->rg_next;
  }
  printf("\n");
  return 0;
}

int print_list_vma(struct vm_area_struct* ivma) {
  struct vm_area_struct* vma = ivma;

  printf("print_list_vma: ");
  if (vma == NULL) { printf("NULL list\n"); return -1; }
  printf("\n");
  while (vma != NULL) {
    printf("va[" FORMAT_ADDR "->" FORMAT_ADDR "]\n", vma->vm_start, vma->vm_end);
    vma = vma->vm_next;
  }
  printf("\n");
  return 0;
}

int print_list_pgn(struct pgn_t* ip) {
  printf("print_list_pgn: ");
  if (ip == NULL) { printf("NULL list\n"); return -1; }
  printf("\n");
  while (ip != NULL) {
    printf("va[" FORMAT_ADDR "]-\n", ip->pgn);
    ip = ip->pg_next;
  }
  printf("n");
  return 0;
}

int print_pgtbl(struct pcb_t *caller, addr_t start, addr_t end)
{
  //  addr_t pgn_start;//, pgn_end;
  //  addr_t pgit;
  //  struct krnl_t *krnl = caller->krnl;

  struct krnl_t *krnl = caller->krnl;
  struct mm_struct *mm = krnl->mm;

  addr_t pgn_start, pgn_end;
  if (end == (addr_t)-1) {
    struct vm_area_struct* cur_vma = get_vma_by_num(mm, 0);
    start = cur_vma->vm_start;
    end = cur_vma->vm_end;
  }

  pgn_start = start >> PAGING64_ADDR_PT_SHIFT;
  pgn_end = end >> PAGING64_ADDR_PT_SHIFT;

  for (addr_t pgit = pgn_start; pgit <= pgn_end; pgit++) {
    addr_t pgd, p4d, pud, pmd, pt;
    get_pd_from_address(pgit << PAGING64_ADDR_PT_SHIFT, &pgd, &p4d, &pud, &pmd, &pt);

    void *pgd_tbl = mm->pgd;
    void *p4d_tbl = mm->p4d;
    void *pud_tbl = mm->pud;
    void *pmd_tbl = mm->pmd;

    printf("print_pgtbl:\n");
    printf("  PDG=%p P4g=%p PUD=%p PMD=%p\n",
           pgd_tbl,
           p4d_tbl,
           pud_tbl,
           pmd_tbl);
  }
  return 0;
}

// int print_pgtbl(struct pcb_t *caller, addr_t start, addr_t end)
// {
//     struct mm_struct *mm = caller->krnl->mm;

//     printf("print_pgtbl:\n");
//     printf(" PDG=%p P4g=%p PUD=%p PMD=%p\n",
//            (void *)mm->pgd,
//            (void *)mm->p4d,
//            (void *)mm->pud,
//            (void *)mm->pmd);

//     return 0;
// }

// int print_pgtbl(struct pcb_t* caller, addr_t start, addr_t end) {
//   addr_t pgn_start, pgn_end;
//   addr_t pgit;
//   struct krnl_t* krnl = caller->krnl;
//   #ifdef DEBUG
//   printf("[DEBUG] print_pgtbl called for PID=%d, start=0x%llx, end=0x%llx\n", caller->pid, (unsigned long long)start, (unsigned long long)end);
//   #endif

//   if (end == (addr_t)-1) {
//     struct vm_area_struct* cur_vma = get_vma_by_num(caller->krnl->mm, 0);
//     start = cur_vma->vm_start;
//     end = cur_vma->vm_end;
//     #ifdef DEBUG
//     printf("[DEBUG] print_pgtbl: using vma0 range start=0x%llx, end=0x%llx\n", (unsigned long long)start, (unsigned long long)end);
//     #endif
//   }

//   pgn_start = start >> PAGING64_ADDR_PT_SHIFT;
//   pgn_end = end >> PAGING64_ADDR_PT_SHIFT;

//   addr_t pgd = 0;
//   addr_t p4d = 0;
//   addr_t pud = 0;
//   addr_t pmd = 0;
//   addr_t pt = 0;

//   for (pgit = pgn_start; pgit <= pgn_end; pgit += PAGING64_PAGESZ) {
//     get_pd_from_address(pgit, &pgd, &p4d, &pud, &pmd, &pt);
//     uint64_t* pgd_tbl = krnl->mm->pgd;
//     if (!pgd_tbl) { 
//       #ifdef DEBUG
//       printf("[DEBUG] print_pgtbl: pgd_tbl NULL\n"); 
//       #endif

//       continue; 
//     }
//     if (pgd_tbl[pgd] == 0) { 
//       #ifdef DEBUG
//       printf("[DEBUG] print_pgtbl: pgd_tbl[%llu] == 0\n", (unsigned long long)pgd); 
//       #endif

//       continue; 
//     }

//     uint64_t* p4d_tbl = (uint64_t*)pgd_tbl[pgd];
//     if (!p4d_tbl) {
//       #ifdef DEBUG
//       printf("[DEBUG] print_pgtbl: p4d_tbl NULL\n"); 
//       #endif

//       continue; 
//     }
//     if (p4d_tbl[p4d] == 0) { 
//       #ifdef DEBUG
//       printf("[DEBUG] print_pgtbl: p4d_tbl[%llu] == 0\n", (unsigned long long)p4d); 
//       #endif

//       continue; 
//     }

//     uint64_t* pud_tbl = (uint64_t*)p4d_tbl[p4d];
//     if (!pud_tbl) { 
//       #ifdef DEBUG
//       printf("[DEBUG] print_pgtbl: pud_tbl NULL\n"); 
//       #endif

//       continue; 
//     }
//     if (pud_tbl[pud] == 0) { 
//       #ifdef DEBUG
//       printf("[DEBUG] print_pgtbl: pud_tbl[%llu] == 0\n", (unsigned long long)pud); 
//       #endif

//       continue; 
//     }

//     uint64_t* pmd_tbl = (uint64_t*)pud_tbl[pud];
//     if (!pmd_tbl) { 
//       #ifdef DEBUG
//       printf("[DEBUG] print_pgtbl: pmd_tbl NULL\n");
//       #endif

//       continue; 
//     }
//     if (pmd_tbl[pmd] == 0) { 
//       #ifdef DEBUG
//       printf("[DEBUG] print_pgtbl: pmd_tbl[%llu] == 0\n", (unsigned long long)pmd); 
//       #endif

//       continue; 
//     }

//     uint64_t* pt_tbl = (uint64_t*)pmd_tbl[pmd];
//     if (!pt_tbl) {
//       #ifdef DEBUG
//       printf("[DEBUG] print_pgtbl: pt_tbl NULL\n");
//       #endif

//       continue; 
//     }

  //   printf("print_pgtbl:\n");
  //   printf("  PDG=%p P4g=%p PUD=%p PMD=%p\n",
  //           (void*)pgd_tbl,
  //           (void*)p4d_tbl,
  //           (void*)pud_tbl,
  //           (void*)pmd_tbl
  //         );
  // }
//   return 0;
// }

#endif  //def MM64
