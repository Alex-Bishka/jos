#include <kern/e1000.h>
#include <kern/pmap.h>
#include <inc/string.h>

static volatile char* e1000_base;
struct tx_desc* tx_desc_arr;
char* send_bufptr[64];
#define E1000_ADDR(offset) (*(volatile uint32_t*) (e1000_base + offset))
#define NUM_DESC 64


// todo:
// 1) change va's to pa's (where e1000 card needs an addr)
// 2) set all fields tx_desc
// 	a) cmd's bits as well
// 3) after 1 and 2 run a dump
// Do we need to set cso/cmd/css/special?
// I think cso and css? is 0b

int
e1000_attach(struct pci_func *pcif)
{
	pci_func_enable(pcif);
	e1000_base = mmio_map_region(pcif->reg_base[0], pcif->reg_size[0]);
	cprintf("Device status register value is: %x\n", E1000_ADDR(E1000_STATUS));
	struct PageInfo* pp = page_alloc(ALLOC_ZERO);
	tx_desc_arr = (struct tx_desc*) page2kva(pp);
	E1000_ADDR(E1000_TDBAL) = page2pa(pp);
	E1000_ADDR(E1000_TDLEN) = 1 << 10; // each queue is size 1k 
	E1000_ADDR(E1000_TDH) = 0;
	E1000_ADDR(E1000_TDT) = 0;
	E1000_ADDR(E1000_TCTL) |= E1000_TCTL_EN | E1000_TCTL_PSP;
	E1000_ADDR(E1000_TCTL) |= 0x10 << 4; // this is to set Collision Threshold to desired value of 10h
	E1000_ADDR(E1000_TCTL) |= 0x40 << 12; // this is to set Collision Distance to expected value of 40h
	E1000_ADDR(E1000_TIPG) |= (0x6 << 20) | (0x8 << 10) | 0xA; 
	
	for (int i = 0; i < NUM_DESC; i += 2) {
		struct PageInfo* pg = page_alloc(ALLOC_ZERO);
		void* pgaddr = page2kva(pg);
		send_bufptr[i] = pgaddr;
		send_bufptr[i+1] = pgaddr + PGSIZE/2;
		tx_desc_arr[i].addr = (uint64_t) ((uint32_t) PADDR(send_bufptr[i]));
		tx_desc_arr[i].cmd |= (1 << 3) | 1; // RS bit and EOP bit
		tx_desc_arr[i].cso = 0; 
		tx_desc_arr[i].status |= 1; // DD bit
		tx_desc_arr[i+1].addr = (uint64_t) ((uint32_t) PADDR(send_bufptr[i+1]));
		tx_desc_arr[i+1].cmd |= (1 << 3) | 1; // RS bit and EOP bit
		tx_desc_arr[i+1].cso = 0; 
		tx_desc_arr[i+1].status |= 1; // DD bit
	}
	return 1;
} 

int
transmit_packet(char* buf, size_t size) {
	struct tx_desc* desc = &tx_desc_arr[E1000_ADDR(E1000_TDT)]; 
	if (!(desc->status & E1000_TXD_STAT_DD)) {
		return -1; // TODO: set to some other error code
	}
	assert(size <= MAX_PACKET_SIZE);
	memmove(send_bufptr[E1000_ADDR(E1000_TDT)], buf, size);
	desc->length = size; // may need to check if size is at least 48 bytes	
	desc->status &= ~1;
	E1000_ADDR(E1000_TDT) = (E1000_ADDR(E1000_TDT) + 1) % NUM_DESC;
	return 0;
}

