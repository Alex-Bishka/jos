/* See COPYRIGHT for copyright information. */

#include <inc/mmu.h>
#include <inc/memlayout.h>
#include <inc/trap.h>



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
#define TRAPHANDLER(name, num)						\
	.globl name;		/* define global symbol for 'name' */	\
	.type name, @function;	/* symbol type is function */		\
	.align 2;		/* align function definition */		\
	name:			/* function starts here */		\
	pushl $(num);							\
	jmp _alltraps

/* Use TRAPHANDLER_NOEC for traps where the CPU doesn't push an error code.
 * It pushes a 0 in place of the error code, so the trap frame has the same
 * format in either case.
 */
#define TRAPHANDLER_NOEC(name, num)					\
	.globl name;							\
	.type name, @function;						\
	.align 2;							\
	name:								\
	pushl $0;							\
	pushl $(num);							\
	jmp _alltraps

.text

/*
 * Lab 3: Your code here for generating entry points for the different traps.
 */

TRAPHANDLER("DivideError", 0);
TRAPHANDLER("NonMaskableInterrupt", 1);
TRAPHANDLER("Breakpoint", 2);
TRAPHANDLER("Overflow", 3);
TRAPHANDLER("BOUNDRangeExceeded", 4);
TRAPHANDLER("InvalidOpCode", 5);
TRAPHANDLER("DeviceNotAvailable", 6);
TRAPHANDLER("DoubleFault", 7);
TRAPHANDLER("CoprocessorSegmentOverrun", 8);
TRAPHANDLER("InvalidTSS", 9);
TRAPHANDLER("SegmentNotPresent", 10);
TRAPHANDLER("StackFault", 11);
TRAPHANDLER("GeneralProtection", 12);
TRAPHANDLER("PageFault", 13);
TRAPHANDLER("UnknownTrap", 14);
TRAPHANDLER("x87FPUFloatingPointError", 15);
TRAPHANDLER("AlignmentCheck", 16);
TRAPHANDLER("MachineCheck", 17);
TRAPHANDLER("SIMDFloatingPointException", 18);

/*
 * Lab 3: Your code here for _alltraps
 */

_alltraps:
	pop %ecx;
	push 0;
	push %ss;
	push %esp;
	pushl %eflags;
	push tf_padding3;
	push %cs;
	push %eip;
	pushl 0;
	pushl %ecx
	push 0;
	push %ds;
	push 0;
	push %es;
	pushal;
	load GD_KD %ds;
	load GD_KD %es;
	pushl %esp;
	call trap;

