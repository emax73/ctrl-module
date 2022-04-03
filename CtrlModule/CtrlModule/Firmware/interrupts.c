#include "interrupts.h"

typedef void (*IrqHandlerType)();

static IrqHandlerType handler[IRQ_MAX] = {0};
int isrdbg;

extern void PS2Handler();

static void IrqHandler() {
	int isr;

	DisableInterrupts();
	isr = GetInterrupts();
	isrdbg ^= isr;

	// if ((isr & ISR_VBLANK) && handler[IRQ_VBLANK])
	// 	handler[IRQ_VBLANK]();

	if ((isr & ISR_PS2) && handler[IRQ_PS2])
		handler[IRQ_PS2]();

	if ((isr & ISR_TAPE) && handler[IRQ_TAPE])
		handler[IRQ_TAPE]();

	// if ((isr & ISR_TAPEIN) && handler[IRQ_TAPEIN])
	// 	handler[IRQ_TAPEIN]();
	//
	// if ((isr & ISR_DISK) && handler[IRQ_DISK])
	// 	handler[IRQ_DISK]();

	EnableInterrupts();
}


extern void (*_inthandler_fptr)();

void InitInterrupts() {
	int i;
	for (i=0; i<IRQ_MAX; i++) {
		handler[i] = 0;
	}
}

void SetIntHandler(int irq, void (*new_handler)()) {
	_inthandler_fptr = IrqHandler;
	if (irq < IRQ_MAX) {
		handler[irq] = new_handler;
	}
}


int GetInterrupts()
{
	return(HW_INTERRUPT(REG_INTERRUPT_CTRL));
}


void EnableInterrupts()
{
	HW_INTERRUPT(REG_INTERRUPT_CTRL)=1;
}


void DisableInterrupts()
{
	HW_INTERRUPT(REG_INTERRUPT_CTRL)=0;
}
