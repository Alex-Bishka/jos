#ifndef JOS_KERN_E1000_H
#define JOS_KERN_E1000_H

#include <kern/pci.h>

int e1000_attach(struct pci_func *pcif);

#define E1000_STATUS   0x00008  /* Device Status - RO */
#define E1000_TCTL 	0x00400	/* TX Control - RW */
#define E1000_TDBAL 	0x03800 /* TX Descriptor Base Address Low - RW */
#define E1000_TDBAH 	0x03804 /* TX Descriptor Base Address High - RW */
#define E1000_TDLEN    0x03808  /* TX Descriptor Length - RW */
#define E1000_TDH      0x03810  /* TX Descriptor Head - RW */
#define E1000_TDT      0x03818  /* TX Descripotr Tail - RW */
#define E1000_TCTL_EN     0x00000002    /* enable tx */
#define E1000_TCTL_PSP    0x00000008    /* pad short packets */

#endif  // SOL >= 6
