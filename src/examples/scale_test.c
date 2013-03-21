#include "zb_transport.h"
#include "zb_packets.h"
#include <stdio.h>

/*
 * scale_test.c
 *
 * A simple test program to act as a scale unit in the system. This part of the system will later be run on the embedded microcontroller boards.
 *
 */

/* in the real implementation, this will be updated through DMA by the ADC peripheral continously. */
static unsigned int DMA_ADC_VALUE;

int main(void) {
	char c;

	zb_packets_init();
	zb_set_broadcast_mode(0);
	zb_set_device_id(4);

	DMA_ADC_VALUE = 128;

	while (1) {
		c = zb_getc();

		switch(zb_parse(c)) {
			case ZB_VALID_PACKET:
				if (zb_packet_op == OP_MEASURE_REQUEST) {
					printf("received measurement request packet");
					zb_send_packet(OP_MEASURE_RESPONSE, "0080", 4);
				} else if (zb_packet_op == OP_PING) {
					printf("received PING\n");
					zb_send_packet(OP_PONG, NULL, 0);
				}
				break;
			default:
				break;
		}
	}

	return 0;
}
