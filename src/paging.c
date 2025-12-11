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

	// --- Extended testcase for liballoc and libfree ---
	int alloc_result, free_result;
	int reg_index;

	// 1. Alloc và free nhiều vùng liên tiếp
	for (reg_index = 1; reg_index <= 3; ++reg_index) {
		printf("[TEST] Allocating 64 bytes for reg_index=%d...\n", reg_index);
		alloc_result = liballoc(proc, 64, reg_index);
		if (alloc_result == 0) {
			printf("[TEST] liballoc succeeded for reg_index=%d\n", reg_index);
		} else {
			printf("[TEST] liballoc FAILED for reg_index=%d\n", reg_index);
		}
	}

	// 2. Free từng vùng
	for (reg_index = 1; reg_index <= 3; ++reg_index) {
		printf("[TEST] Freeing region for reg_index=%d...\n", reg_index);
		free_result = libfree(proc, reg_index);
		if (free_result == 0) {
			printf("[TEST] libfree succeeded for reg_index=%d\n", reg_index);
		} else {
			printf("[TEST] libfree FAILED for reg_index=%d\n", reg_index);
		}
	}

	// 3. Thử free lại vùng đã free (nên báo lỗi)
	printf("[TEST] Freeing already freed region reg_index=1...\n");
	free_result = libfree(proc, 1);
	if (free_result == 0) {
		printf("[TEST] libfree (unexpectedly) succeeded for reg_index=1\n");
	} else {
		printf("[TEST] libfree correctly failed for already freed reg_index=1\n");
	}

	// 4. Alloc lại vùng đã free (nên được cấp phát lại)
	printf("[TEST] Allocating 32 bytes for reg_index=1 again...\n");
	alloc_result = liballoc(proc, 32, 1);
	if (alloc_result == 0) {
		printf("[TEST] liballoc succeeded for reg_index=1 (reuse)\n");
	} else {
		printf("[TEST] liballoc FAILED for reg_index=1 (reuse)\n");
	}

	// 5. Free vùng vừa alloc lại
	printf("[TEST] Freeing region for reg_index=1 (reuse)...\n");
	free_result = libfree(proc, 1);
	if (free_result == 0) {
		printf("[TEST] libfree succeeded for reg_index=1 (reuse)\n");
	} else {
		printf("[TEST] libfree FAILED for reg_index=1 (reuse)\n");
	}
	// --- End extended testcase ---

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

