// Stub for krnl_struct to satisfy minimal test
struct krnl_struct { struct mm_struct *mm; };


#include "libmem.h"
#include <stdio.h>


// --- Stubs for minimal test ---
struct pcb_t *load(const char *path) {
	static struct pcb_t pcb;
	static struct mm_struct mm;
	static struct krnl_t krnl;
	static struct vm_area_struct vma;
	static struct code_seg_t code;
	static struct inst_t insts[1];
	static struct vm_rg_struct symrgtbl[PAGING_MAX_SYMTBL_SZ+1];
	// Khởi tạo các trường cần thiết để tránh segfault
	mm.mmap = &vma;
	mm.symrgtbl[0].rg_start = 0;
	mm.symrgtbl[0].rg_end = 0;
	mm.fifo_pgn = NULL;
	vma.vm_id = 0;
	vma.vm_start = 0;
	vma.vm_end = 0;
	vma.sbrk = 0;
	vma.vm_mm = &mm;
	vma.vm_freerg_list = NULL;
	vma.vm_next = NULL;
	mm.symrgtbl[0].rg_next = NULL;
	mm.symrgtbl[1].rg_next = NULL;
	krnl.mm = &mm;
	krnl.mram = NULL;
	krnl.mswp = NULL;
	krnl.active_mswp = NULL;
	krnl.active_mswp_id = 0;
	pcb.krnl = &krnl;
	pcb.pid = 1;
	pcb.priority = 0;
	pcb.code = &code;
	code.text = insts;
	code.size = 1;
	pcb.pc = 0;
	pcb.page_table = NULL;
	pcb.bp = 0;
	return &pcb;
}

struct vm_area_struct *get_vma_by_num(struct mm_struct *mm, int vmaid) {
	static struct vm_area_struct vma;
	return &vma;
}

void print_pgtbl(struct pcb_t *proc, int vmaid, int max) {}
void *pte_get_entry() { return NULL; }
int MEMPHY_get_freefp() { return 0; }
void enlist_pgn_node() {}
void MEMPHY_put_freefp() {}
void MEMPHY_dump() {}


int main() {
	struct pcb_t * ld = load("input/p0");
	struct pcb_t * proc = load("input/p0");

	// --- 10 Testcase alloc/free (bao gồm edge case) ---
	int alloc_result, free_result;
	int reg_index;

	// 1. Alloc liên tục 5 vùng, size tăng dần
	for (reg_index = 1; reg_index <= 5; ++reg_index) {
		printf("[TC1] Allocating %d bytes for reg_index=%d...\n", reg_index*16, reg_index);
		alloc_result = liballoc(proc, reg_index*16, reg_index);
		printf("[TC1] %s for reg_index=%d\n", alloc_result==0?"SUCCESS":"FAIL", reg_index);
	}

	// 2. Free xen kẽ các vùng vừa alloc
	for (reg_index = 1; reg_index <= 5; reg_index+=2) {
		printf("[TC2] Freeing region for reg_index=%d...\n", reg_index);
		free_result = libfree(proc, reg_index);
		printf("[TC2] %s for reg_index=%d\n", free_result==0?"SUCCESS":"FAIL", reg_index);
	}

	// 3. Double free vùng đã free
	printf("[TC3] Double free reg_index=1...\n");
	free_result = libfree(proc, 1);
	printf("[TC3] %s for reg_index=1\n", free_result==0?"UNEXPECTED SUCCESS":"CORRECT FAIL");

	// 4. Alloc với reg_index biên (0 và PAGING_MAX_SYMTBL_SZ)
	printf("[TC4] Allocating 32 bytes for reg_index=0 (edge)...\n");
	alloc_result = liballoc(proc, 32, 0);
	printf("[TC4] %s for reg_index=0\n", alloc_result==0?"SUCCESS":"FAIL");
	printf("[TC4] Allocating 32 bytes for reg_index=%d (edge)...\n", PAGING_MAX_SYMTBL_SZ);
	alloc_result = liballoc(proc, 32, PAGING_MAX_SYMTBL_SZ);
	printf("[TC4] %s for reg_index=%d\n", alloc_result==0?"SUCCESS":"FAIL", PAGING_MAX_SYMTBL_SZ);

	// 5. Free với reg_index biên
	printf("[TC5] Freeing region for reg_index=0 (edge)...\n");
	free_result = libfree(proc, 0);
	printf("[TC5] %s for reg_index=0\n", free_result==0?"SUCCESS":"FAIL");
	printf("[TC5] Freeing region for reg_index=%d (edge)...\n", PAGING_MAX_SYMTBL_SZ);
	free_result = libfree(proc, PAGING_MAX_SYMTBL_SZ);
	printf("[TC5] %s for reg_index=%d\n", free_result==0?"SUCCESS":"FAIL", PAGING_MAX_SYMTBL_SZ);

	// 6. Alloc với size=0
	printf("[TC6] Allocating 0 bytes for reg_index=6...\n");
	alloc_result = liballoc(proc, 0, 6);
	printf("[TC6] %s for reg_index=6\n", alloc_result==0?"SUCCESS":"FAIL");

	// 7. Free với reg_index chưa từng alloc
	printf("[TC7] Freeing region for reg_index=7 (never alloc)...\n");
	free_result = libfree(proc, 7);
	printf("[TC7] %s for reg_index=7\n", free_result==0?"UNEXPECTED SUCCESS":"CORRECT FAIL");

	// 8. Alloc với reg_index trùng (đã alloc trước đó, chưa free)
	printf("[TC8] Allocating 64 bytes for reg_index=2 (already allocated)...\n");
	alloc_result = liballoc(proc, 64, 2);
	printf("[TC8] %s for reg_index=2\n", alloc_result==0?"SUCCESS (should reuse or overwrite)":"FAIL");

	// 9. Alloc vượt giới hạn reg_index
	printf("[TC9] Allocating 32 bytes for reg_index=%d (out of bound)...\n", PAGING_MAX_SYMTBL_SZ+1);
	alloc_result = liballoc(proc, 32, PAGING_MAX_SYMTBL_SZ+1);
	printf("[TC9] %s for reg_index=%d\n", alloc_result==0?"UNEXPECTED SUCCESS":"CORRECT FAIL", PAGING_MAX_SYMTBL_SZ+1);

	// 10. Free vượt giới hạn reg_index
	printf("[TC10] Freeing region for reg_index=%d (out of bound)...\n", PAGING_MAX_SYMTBL_SZ+1);
	free_result = libfree(proc, PAGING_MAX_SYMTBL_SZ+1);
	printf("[TC10] %s for reg_index=%d\n", free_result==0?"UNEXPECTED SUCCESS":"CORRECT FAIL", PAGING_MAX_SYMTBL_SZ+1);
	// --- End 10 testcase ---

	// --- Commented out: simulation logic, not needed for liballoc/libfree test ---
	/*
	unsigned int i;
	for (i = 0; i < proc->code->size; i++) {
		run(proc);
		run(ld);
	}
	dump();
	*/
	return 0;
}

