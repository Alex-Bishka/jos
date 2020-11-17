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
	return 1;
}
