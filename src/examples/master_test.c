#define DEVICE_ID 0

#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <ctype.h>
#include "zb_transport.h"
#include "zb_packets.h"
#include "requesthandlers.h"


/*
 * master_test.c
 *
 * simple test application for sending, receiving, and parsing packets as the master unit.
 * This will later be integrated with a web-server for a friendlier user interface.
 *
 * Reads commands from standard input to emulate asynchronously appearing HTTP requests.
 *
 * Author: Kristian Hentschel
 * Team Project 3. University of Glasgow. 2013
 */


static void *thread_parse(void *);

/* for testing only. will be replaced by webserver implementation. */
int main(void) {
	char c;
	pthread_t parser_thread;
	char response_buffer[REQUEST_RESULT_BUFSIZE];

	pthread_create(&parser_thread, NULL, thread_parse, NULL);

	zb_packets_init();
	zb_set_broadcast_mode(1);
	zb_set_device_id(0);

	while ((c = getchar()) != 'q') {
		if (!isalpha((int) c)) {
			continue;
		}
		switch (c) {
			case 'm':
				REQUEST_measure(response_buffer);
				break;
			case 'c':
				REQUEST_calibrate(response_buffer);
				break;
			case 'd':
				REQUEST_data(response_buffer);
				break;
			case 'p':
				REQUEST_ping(response_buffer);
				break;
			case 'I':
				printf("Sending ATNI node identity command\n");
				zb_send_command("NI");
				break;
			case 'D':
				printf("Sending ATND node discover command\n");
				zb_send_command("ND");
				break;
			default:
				printf("unknown command %c\n", c);
		}
	}

	zb_transport_stop();	
	
	printf("good-bye\n");
	return 0;
}
	
/* this could/should be a separate thread. */
static void *thread_parse(void *arg) {
	char c;

	while(1){
		c = zb_getc();
		printf("%02x ", c);
		fflush(stdout);

		switch (zb_parse(c)) {
			case ZB_START_PACKET:
				printf("\n(start of packet)\n");
				break;
			case ZB_PLAIN_WORD:
				printf("\n(plain word of %d characters)\n", zb_word_len);
				break;
			case ZB_VALID_PACKET:
				printf("\n(valid packet of %d characters with op code %x from device %x: '%s')\n", zb_packet_len, zb_packet_op, zb_packet_from, strndup(zb_packet_data, zb_packet_len));
				HANDLE_packet_received();
				break;
			case ZB_INVALID_PACKET:
				printf("\n(invalid packet)\n");
				break;
			default:
				break;
		}
	}
	return NULL;
}
