/* See COPYRIGHT for copyright information. */

#include <inc/mmu.h>
#include <inc/memlayout.h>
#include <inc/trap.h>

#include <kern/picirq.h>


###################################################################
# exceptions/interrupts
###################################################################

/* TRAPHANDLER defines a globally-visible function for handling a trap.
 * It pushes a trap number onto the stack, then jumps to _alltraps.
 * Use TRAPHANDLER for traps where the CPU automatically pushes an error code.
 *
 * You shouldn't call a TRAPHANDLER function from C, but you may
 * need to _declare_ one in C (for instance, to get a function pointer
 * during IDT setup).  You can declare the function with
 *   void NAME();
 * where NAME is the argument passed to TRAPHANDLER.
 */
#define TRAPHANDLER(name, num, dpl)					;\
.text									;\
	.globl name		/* define global symbol for 'name' */	;\
	.type name, @function	/* symbol type is function */		;\
	.align 2		/* align function definition */		;\
	name:			/* function starts here */		;\
	pushl $(num)							;\
	jmp _alltraps							;\
.data								        ;\
	.long name 							;\
	.long num 							;\
	.long dpl							;\
	.long 1
/* Use TRAPHANDLER_NOEC for traps where the CPU doesn't push an error code.
 * It pushes a 0 in place of the error code, so the trap frame has the same
 * format in either case.
 */
#define TRAPHANDLER_NOEC(name, num, dpl)				;\
.text									;\
	.globl name							;\
	.type name, @function						;\
	.align 2							;\
	name:								;\
	pushl $0							;\
	pushl $(num)							;\
	jmp _alltraps 							;\
.data 									;\
	.long name 							;\
	.long num 							;\
	.long dpl 							;\
	.long 1

.data
	.globl traphandlers

traphandlers:
TRAPHANDLER_NOEC(DivideError, T_DIVIDE, 0);
TRAPHANDLER_NOEC(NonMaskableInterrupt, T_NMI, 0);
TRAPHANDLER_NOEC(Breakpoint, T_BRKPT, 3);
TRAPHANDLER_NOEC(Overflow, T_OFLOW, 0);
TRAPHANDLER_NOEC(BOUNDRangeExceeded, T_BOUND, 0);
TRAPHANDLER_NOEC(InvalidOpCode, T_ILLOP, 0);
TRAPHANDLER_NOEC(DeviceNotAvailable, T_DEVICE, 0);
TRAPHANDLER(DoubleFault, T_DBLFLT, 0);
TRAPHANDLER(InvalidTSS, T_TSS, 0);
TRAPHANDLER(SegmentNotPresent, T_SEGNP, 0);
TRAPHANDLER(StackFault, T_STACK, 0);
TRAPHANDLER(GeneralProtection, T_GPFLT, 0);
TRAPHANDLER(PageFault, T_PGFLT, 0);
TRAPHANDLER_NOEC(x87FPUFloatingPointError, T_FPERR, 0);
TRAPHANDLER_NOEC(AlignmentCheck, T_ALIGN, 0);
TRAPHANDLER_NOEC(MachineCheck, T_MCHK, 0);
TRAPHANDLER_NOEC(SIMDFloatingPointException, T_SIMDERR, 0);
TRAPHANDLER_NOEC(SysCall, T_SYSCALL, 3);
TRAPHANDLER_NOEC(TIMER, (IRQ_OFFSET + IRQ_TIMER), 0);
TRAPHANDLER_NOEC(KBD, IRQ_OFFSET + IRQ_KBD, 0);
TRAPHANDLER_NOEC(SERIAL, IRQ_OFFSET + IRQ_SERIAL, 0);
TRAPHANDLER_NOEC(SPURIOUS, IRQ_OFFSET + IRQ_SPURIOUS, 0);
TRAPHANDLER_NOEC(IDE, IRQ_OFFSET + IRQ_IDE, 0);
TRAPHANDLER_NOEC(ERROR, IRQ_OFFSET + IRQ_ERROR, 0);
.data
	// our null pointer that signifies the end of the IDT array
	.long 0
	.long 0
	.long 0
	.long 0
.text
_alltraps:
	pushl %ds;
	pushl %es;
	pushal;
	pushl $GD_KD;
	popl %ds;
	pushl $GD_KD;
	popl %es;
	pushl %esp;
	call trap;

