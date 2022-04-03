#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#define INTERRUPTBASE 0xffffffb0
#define HW_INTERRUPT(x) *(volatile unsigned int *)(INTERRUPTBASE+x)

// Interrupt control register
// Write a '1' to the low bit to enable interrupts, '0' to disable.
// Reading returns a set bit for each interrupt that has been triggered since
// the last read, and also clears the register.

#define REG_INTERRUPT_CTRL 0x0

// #define IRQ_VBLANK    0
#define IRQ_PS2       0
#define IRQ_TAPE      1
// #define IRQ_TAPEIN    2
#define IRQ_MAX       2

// #define ISR_VBLANK    0x01
#define ISR_PS2       0x01
#define ISR_TAPE      0x02
// #define ISR_TAPEIN    0x08
// #define ISR_DISK      0x04


void SetIntHandler(int irq, void(*handler)());
void EnableInterrupts();
void DisableInterrupts();
int GetInterrupts();
void InitInterrupts();

#endif
