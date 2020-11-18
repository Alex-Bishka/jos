#include <kern/e1000.h>
#include <kern/pmap.h>
#include <inc/string.h>

static volatile char* e1000_base;
#define E1000_ADDR(offset) (*(volatile uint32_t*) (e1000_base + offset))

// LAB 6: Your driver code here
int
e1000_attach(struct pci_func *pcif)
{
	pci_func_enable(pcif);
	e1000_base = mmio_map_region(pcif->reg_base[0], pcif->reg_size[0]);
	cprintf("Device status register value is: %x\n", E1000_ADDR(E1000_STATUS));
	struct PageInfo * pp = page_alloc(ALLOC_ZERO);
	E1000_ADDR(E1000_TDBAL) = (uint32_t) page2kva(pp);
	E1000_ADDR(E1000_TDLEN) = 1 << 10; // each queue is size 1k 
	E1000_ADDR(E1000_TDH) = 0;
	E1000_ADDR(E1000_TDT) = 0;
	E1000_ADDR(E1000_TCTL) |= E1000_TCTL_EN | E1000_TCTL_PSP;
	E1000_ADDR(E1000_TCTL) |= 0x10 << 4; // this is to set Collision Threshold to desired value of 10h
	E1000_ADDR(E1000_TCTL) |= 0x40 << 12; // this is to set Collision Distance to expected value of 40h

	return 1;
} 
