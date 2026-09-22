/* cpu/idt.h */
#ifndef IDT_H
#define IDT_H
#include "../kernel/kernel.h"

typedef struct
{
       uint16_t offset_low;
       uint16_t selector; /* kernel code segment = 0x08 */
       uint8_t zero;
       uint8_t type_attr; /* 0x8E = present, ring0, 32-bit interrupt gate */
       uint16_t offset_high;
} PACKED idt_entry_t;

typedef struct
{
       uint16_t limit;
       uint32_t base;
} PACKED idt_ptr_t;

void idt_init(void);
void idt_set_gate(uint8_t num, uint32_t handler,
                  uint16_t sel, uint8_t flags);
#endif
