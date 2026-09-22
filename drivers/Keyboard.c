/* drivers/keyboard.c
 * PS/2 keyboard driver using IRQ 1 (vector 33). */

#include "keyboard.h"
#include "vga.h"
#include "../cpu/irq.h"
#include "../lib/kprintf.h"

static const char sc_ascii[] = {
    0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
    '*', 0, ' '
};

static const char sc_ascii_shift[] = {
    0, 0, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
    0, '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,
    '*', 0, ' '
};

#define KB_BUF_SIZE 64
static char kb_buf[KB_BUF_SIZE];
static uint8_t kb_head = 0;
static uint8_t kb_tail = 0;

static void buf_push(char c)
{
       uint8_t next = (kb_tail + 1) % KB_BUF_SIZE;
       if (next == kb_head)
              return;
       kb_buf[kb_tail] = c;
       kb_tail = next;
}

static int buf_pop(char *out)
{
       if (kb_head == kb_tail)
              return 0;
       *out = kb_buf[kb_head];
       kb_head = (kb_head + 1) % KB_BUF_SIZE;
       return 1;
}

static uint8_t shift_held = 0;
static uint8_t caps_lock = 0;

static void keyboard_irq_handler(registers_t *r)
{
       UNUSED(r);
       uint8_t sc = inb(KB_DATA_PORT);

       uint8_t released = sc & KB_RELEASED;
       uint8_t code = sc & ~KB_RELEASED;

       if (code == 0x2A || code == 0x36)
       {
              shift_held = !released;
              return;
       }
       if (code == 0x3A && !released)
       {
              caps_lock ^= 1;
              return;
       }

       if (released)
              return;

       if (code >= ARRAY_SIZE(sc_ascii))
              return;

       char c = shift_held ? sc_ascii_shift[code] : sc_ascii[code];
       if (!c)
              return;

       if (caps_lock)
       {
              if (c >= 'a' && c <= 'z')
                     c -= 32;
              else if (c >= 'A' && c <= 'Z')
                     c += 32;
       }

       buf_push(c);
}

void keyboard_init(void)
{
       irq_register_handler(1, keyboard_irq_handler);
       kprintf("[KB] PS/2 keyboard driver ready\n");
}

int keyboard_haschar(void)
{
       return kb_head != kb_tail;
}

char keyboard_getchar(void)
{
       char c;
       while (!buf_pop(&c))
              asm volatile("hlt");
       return c;
}
