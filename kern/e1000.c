#include <kern/e1000.h>
#include <kern/pmap.h>
#include <inc/string.h>

static volatile char* e1000_base;
struct tx_desc* tx_desc_arr;
struct rx_desc* rx_desc_arr;
char* send_bufptr[64];
char* recv_bufptr[128];
#define E1000_ADDR(offset) (*(volatile uint32_t*) (e1000_base + offset))
#define NUM_TX_DESC 64
#define NUM_RX_DESC 128


// todo:
// 1) RAL/RAH?
// 2) What should RDT/RDH be set to? 
// 3) RDMTS?
// 4) MO?

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
	
	for (int i = 0; i < NUM_TX_DESC; i += 2) {
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
	struct PageInfo* pp_rx = page_alloc(ALLOC_ZERO);
	rx_desc_arr = (struct rx_desc*) page2kva(pp_rx);
	E1000_ADDR(E1000_MTA) = 0;
	
	uint32_t mac_addr[2];
	get_mac_addr(mac_addr);
	
	E1000_ADDR(E1000_RAL) = mac_addr[0];
	E1000_ADDR(E1000_RAH) = mac_addr[1] | (1 << 31); // includes present bit
	E1000_ADDR(E1000_RDBAL) = page2pa(pp_rx);
	E1000_ADDR(E1000_RDLEN) = 1 << 11; // each queue is size 1k 
	E1000_ADDR(E1000_RDH) = 1; // double check this
	E1000_ADDR(E1000_RDT) = 0;
	E1000_ADDR(E1000_RCTL) |= E1000_RCTL_BAM | E1000_RCTL_SECRC;
	
	for (int i = 0; i < NUM_RX_DESC; i += 2) {
		struct PageInfo* pg = page_alloc(ALLOC_ZERO);
		void* pgaddr = page2kva(pg);
		recv_bufptr[i] = pgaddr;
		recv_bufptr[i+1] = pgaddr + PGSIZE/2;
		rx_desc_arr[i].addr = (uint64_t) ((uint32_t) PADDR(recv_bufptr[i]));
		rx_desc_arr[i].status |= (1 << 1); // EOP and DD bit
		rx_desc_arr[i+1].addr = (uint64_t) ((uint32_t) PADDR(recv_bufptr[i+1]));
		rx_desc_arr[i+1].status |= (1 << 1); // EOP and DD bit
	}
	E1000_ADDR(E1000_RCTL) |= E1000_RCTL_EN;
	return 1;
} 

int
get_mac_addr(uint32_t* addr) {
	volatile uint32_t *eeprom = &E1000_ADDR(E1000_EERD);
	*eeprom = (0 << 8) | 1;
	while (! (*eeprom && (1 << 4))) {}
	addr[0] = (*eeprom >> 16);
	*eeprom = (1 << 8) | 1;
	while (! (*eeprom && (1 << 4))) {}
	addr[0] |= (*eeprom & 0xffff0000);
	*eeprom = (2 << 8) | 1;
	while (! (*eeprom && (1 << 4))) {}
	addr[1] = (*eeprom >> 16);
	return 0;
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
	desc->status &= ~E1000_TXD_STAT_DD;
	E1000_ADDR(E1000_TDT) = (E1000_ADDR(E1000_TDT) + 1) % NUM_TX_DESC;
	return 0;
}

int
receive_packet(char* buf) {
	int rdt = (E1000_ADDR(E1000_RDT) + 1) % NUM_RX_DESC;
	struct rx_desc* desc = &rx_desc_arr[rdt];
	if (!(desc->status & E1000_RXD_STAT_DD)) {
		return -1;
	}
	assert(desc->status & E1000_RXD_STAT_EOP);
	memmove(buf, recv_bufptr[rdt], desc->length);
	desc->status &= ~E1000_RXD_STAT_DD;
	E1000_ADDR(E1000_RDT) = rdt;
	return desc->length;
}





