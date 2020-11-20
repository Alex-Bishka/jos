#include "ns.h"
#include "inc/lib.h"

extern union Nsipc nsipcbuf;

void
output(envid_t ns_envid)
{
	binaryname = "ns_output";

	while(true) {
		sys_ipc_recv((void*) REQVA);
		struct jif_pkt* pkt = (struct jif_pkt*) REQVA;
		sys_transmit_packet(pkt->jp_data, pkt->jp_len);
	}
	// LAB 6: Your code here:
	// 	- read a packet from the network server
	//	- send the packet to the device driver
}
