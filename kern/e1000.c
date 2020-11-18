#include <kern/e1000.h>
#include <kern/pmap.h>
#include <inc/string.h>

static volatile char* e1000_base;
#define E1000_ADDR(offset) (*(volatile uint32_t*) (e1000_base + offset))
char* bufptr[64];

#define MAX_PACKET_SIZE 1518

// todo:
// 1) rename bufptr
// 2) save kva of pp (global)
// 3) fix first line of xmit packet
// 4) set the len field of each descriptor inside transmit packet (=size)
// 5) change 64 to be numdesc
// 6) map addr of each descriptor to each buffer


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
	
	for (int i = 0; i < 64; i += 2) {
		struct PageInfo* pg = page_alloc(ALLOC_ZERO);
		void* pgaddr = page2kva(pg);
		bufptr[i] = pgaddr;
		bufptr[i+1] = pgaddr + PGSIZE/2;
		tx_descarray[i].addr = bufptr[i];
		tx_descar[i+1].addr = bufptr[i+1];
	}
	return 1;
} 

int
transmit_packet(char* buf, size_t size) {
	struct tx_desc* next_desc = ((volatile struct tx_desc*) e1000_base + E1000_TDT*sizeof(struct tx_desc));
	if (!(next_desc->status & E1000_TXD_STAT_DD)) {
		return -1; //some other error myabe?
	}
	assert(size <= MAX_PACKET_SIZE);
	memmove(bufptr[E1000_TDT], buf, size);

	E1000_ADDR(E1000_TDT) = (E1000_ADDR(E1000_TDT) + 1) % 64;
}

