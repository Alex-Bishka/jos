#ifndef JOS_KERN_E1000_H
#define JOS_KERN_E1000_H

#include <kern/pci.h>

int e1000_attach(struct pci_func *pcif);
int transmit_packet(char* buf, size_t size);

#define MAX_PACKET_SIZE	0x1518	/* Max size of TX packet */

#define E1000_STATUS   0x00008  /* Device Status - RO */
#define E1000_TCTL 	0x00400	/* TX Control - RW */
#define E1000_TIPG     0x00410  /* TX Inter-packet gap -RW */
#define E1000_TDBAL 	0x03800 /* TX Descriptor Base Address Low - RW */
#define E1000_TDBAH 	0x03804 /* TX Descriptor Base Address High - RW */
#define E1000_TDLEN    0x03808  /* TX Descriptor Length - RW */
#define E1000_TDH      0x03810  /* TX Descriptor Head - RW */
#define E1000_TDT      0x03818  /* TX Descripotr Tail - RW */
#define E1000_TCTL_EN     0x00000002    /* enable tx */
#define E1000_TCTL_PSP    0x00000008    /* pad short packets */

#define E1000_TXD_STAT_DD    0x00000001 /* Descriptor Done */


struct tx_desc
{
	uint64_t addr;
	uint16_t length;
	uint8_t cso;
	uint8_t cmd;
	uint8_t status;
	uint8_t css;
	uint16_t special;
};

#endif  // SOL >= 6
