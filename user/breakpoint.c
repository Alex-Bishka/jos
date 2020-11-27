// program to cause a breakpoint trap

#include <inc/lib.h>

void
umain(int argc, char **argv)
{
	cprintf("Starting function\n");
	asm volatile("int $3");
	cprintf("between breakpoints\n");
	asm volatile("int $3");
	cprintf("After second breakpoint\n");
}

