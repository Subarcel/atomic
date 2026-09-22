/* mm/paging.c
 * Sets up a simple identity-mapped page directory for the kernel.
 * Virtual address == physical address for the first 4 MB.
 * Enables paging by setting CR0.PG. */

#include "paging.h"
#include "pmm.h"
#include "lib/string.h"
#include "lib/kprintf.h"
#include "cpu/isr.h"

/* Page directory and one page table = 4 KB aligned in BSS */
static uint32_t page_dir[1024] __attribute__((aligned(4096)));
static uint32_t page_tab0[1024] __attribute__((aligned(4096)));

/* Page fault handler (ISR 14) */
static void page_fault_handler(registers_t *r)
{
       uint32_t fault_addr;
       asm volatile("mov %%cr2, %0" : "=r"(fault_addr));
       kprintf("[PF] fault at 0x%x  err=0x%x  eip=0x%x\n",
               fault_addr, r->err_code, r->eip);
       panic_set_regs(r);
       panic("Page fault");
}

void paging_init(void)
{
       memset(page_dir, 0, sizeof(page_dir));
       memset(page_tab0, 0, sizeof(page_tab0));

       /* Identity-map the first 4 MB (1024 pages × 4 KB) */
       for (int i = 0; i < 1024; i++)
              page_tab0[i] = (uint32_t)(i * PAGE_SIZE) | PAGE_PRESENT | PAGE_WRITE;

       /* Point directory entry 0 at our page table */
       page_dir[0] = (uint32_t)page_tab0 | PAGE_PRESENT | PAGE_WRITE;

       /* Load CR3 and enable paging via CR0.PG */
       asm volatile(
           "mov %0, %%cr3\n"
           "mov %%cr0, %%eax\n"
           "or  $0x80000000, %%eax\n"
           "mov %%eax, %%cr0\n" ::"r"(page_dir) : "eax");

       isr_register_handler(14, page_fault_handler);
       kprintf("[PAGING] identity-mapped 0–4 MB, CR0.PG set\n");
}

void paging_map(uint32_t virt, uint32_t phys, uint32_t flags)
{
       uint32_t pdi = virt >> 22;
       uint32_t pti = (virt >> 12) & 0x3FF;

       if (!(page_dir[pdi] & PAGE_PRESENT))
       {
              uint32_t frame = pmm_alloc_frame();
              if (!frame)
                     panic("paging_map: OOM");
              memset((void *)frame, 0, PAGE_SIZE);
              page_dir[pdi] = frame | PAGE_PRESENT | PAGE_WRITE;
       }

       uint32_t *tab = (uint32_t *)(page_dir[pdi] & ~0xFFF);
       tab[pti] = phys | flags | PAGE_PRESENT;
}

uint32_t paging_virt_to_phys(uint32_t virt)
{
       uint32_t pdi = virt >> 22;
       uint32_t pti = (virt >> 12) & 0x3FF;
       if (!(page_dir[pdi] & PAGE_PRESENT))
              return 0;
       uint32_t *tab = (uint32_t *)(page_dir[pdi] & ~0xFFF);
       return (tab[pti] & ~0xFFF) + (virt & 0xFFF);
}
