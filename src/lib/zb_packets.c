#include "zb_packets.h"
#include "zb_transport.h"
#include "diagnostics.h"
#include <string.h>
#include <ctype.h>
#include <stdio.h>

/* TODO define this somewhere more sensible, maybe in its own header file? */
#ifndef DEVICE_ID
#define DEVICE_ID 0x02
#endif

#define PACKET_DELIMETER 0x7E
/*
 * zb_packets.h
 *
 * Author: Kristian Hentschel
 * Team Project 3. University of Glasgow. 2013
 *
 * Implementation of a simple packetization layer.
 *
 * This file is redundant, and the implementation has not yet been updated for the recent
 * API changes. The packet structure used herein relies on using radios in AT mode, and is
 * simpler but less robust than the API mode implementation.
 *
 */

/* global variables as declared in .h file */

char	zb_word_data[MAX_PACKET_SIZE];
int		zb_word_len;

char	zb_packet_data[MAX_PACKET_SIZE];
char	zb_packet_op;
char	zb_packet_from;
char	zb_packet_len;

/* utility fucntion to calculate checksum */
static char zb_checksum(char *buf, unsigned char len);

/*
 * send command mode sequence within guard times of 1 second silence.
 *
 * after this, the word OK should be received and the device will accept AT commands.
 * Must exit command mode by sending ATCN command.
 */
void zb_enter_command_mode() {
	zb_guard_delay();
	zb_send("+++", 3);
	zb_guard_delay();
	DIAGNOSTICS("entered command mode\n");
}

/*
 * sends data verbatim for use with AT commands:
 * "AT" cmd data "\r\n"
 * data can be NULL, in that case a command without parameter (read or action) is sent.
 */
void zb_send_command_with_argument(char cmd[2], char *data, unsigned char len) {
	char buf[MAX_PACKET_SIZE];
	unsigned char n, i;

	n = 0;
	buf[n++] = 'A'; 
	buf[n++] = 'T';
	buf[n++] = cmd[0];
	buf[n++] = cmd[1];

	if (data != NULL) {
		buf[n++] = ' ';
		for (i = 0; i < len; i++) {
			buf[n++] = data[i]; 
		}
	}

	buf[n++] = '\r';
	buf[n++] = '\n';
	zb_send(buf, n);
	
	buf[n] = '\0';
	DIAGNOSTICS("sent command %s\n", buf);
}

/*
 * wrapper to send a simple command without an argument
 */
void zb_send_command(char cmd[2]) {
	zb_send_command_with_argument(cmd, NULL, 0);
}

/*
 * assembles a full packet with sender address, length, checksum
 */
void zb_send_packet(char op, char *data, char len) {
	char buf[MAX_PACKET_SIZE];
	unsigned char n, i, chk;

	n = 0;
	buf[n++] = PACKET_DELIMETER;
	buf[n++] = op;
	buf[n++] = DEVICE_ID;
	buf[n++] = len;
	
	for (i = 0; i < len; i++) {
		buf[n] = data[i];
		n++;
	}
	
	chk = zb_checksum(buf, n);
	buf[n++] = chk;
	buf[n++] = '\n';

	zb_send(buf, n);

	DIAGNOSTICS("sent packet of %d bytes.\n", n);
}

/*
 * checksum: sum of all bytes except checksum and initial delimeter.
 * keeping only lower 8 bits, subtract from 0xFF.
 */
char zb_checksum(char *buf, unsigned char len) {
	unsigned char i, result;

	result = 0;
	/* start counting at one to skip packet delimeter byte. */
	for (i = 1; i < len; i++) {
		result += buf[i];
	}

	return 0xFF - result;
}

/*
 * TODO: It's complicated.
 *
 * for return values see header file comment.
 *
 * keep a lot of state in static variables local to this function.
 *
 * store results in global variables defined in header file.
 */
enum zb_parse_state {LEX_WAITING, LEX_IN_WORD, LEX_PACKET_OP, LEX_PACKET_FROM, LEX_PACKET_LENGTH, LEX_PACKET_DATA, LEX_PACKET_CHECKSUM};

enum zb_parse_response zb_parse(char c) {
	static unsigned char checksum;
	static unsigned char packet_data_count;
	static enum zb_parse_state state = LEX_WAITING;

	/* see the start of a packet - discard everything else.
	 * the delimeter character is illegal in data except in a checksum.
	 */

	if (state != LEX_PACKET_CHECKSUM && c == PACKET_DELIMETER) {
		state = LEX_PACKET_OP;
		checksum = 0;
		packet_data_count = 0;
		
		zb_packet_op = 0;
		zb_packet_from = 0;
		zb_packet_len = 0;

		return ZB_START_PACKET;
	}

	switch (state) {
		case LEX_WAITING:
			if (isalnum((int) c)) {
				zb_word_data[0] = c;
				zb_word_len = 1;
				state = LEX_IN_WORD;
			}
			break;
		case LEX_IN_WORD:
			if (isalnum((int) c)) {
				zb_word_data[zb_word_len++] = c;
				state = LEX_IN_WORD;
			} else {
				state = LEX_WAITING;
				return ZB_PLAIN_WORD;
			}
			break;
		case LEX_PACKET_OP:
			zb_packet_op = c;
			checksum += c;
			state = LEX_PACKET_FROM;
			break;
		case LEX_PACKET_FROM:
			zb_packet_from = c;
			checksum += c;
			state = LEX_PACKET_LENGTH;
			break;
		case LEX_PACKET_LENGTH:
			zb_packet_len = c;
			checksum += c;
			if (zb_packet_len == 0) {
				state = LEX_PACKET_CHECKSUM;
			} else {
				state = LEX_PACKET_DATA;
			}
			break;
		case LEX_PACKET_DATA:
			zb_packet_data[packet_data_count++] = c;
			checksum += c;
			if (packet_data_count == zb_packet_len) {
				state = LEX_PACKET_CHECKSUM;
			} else {
				state = LEX_PACKET_DATA;
			}
			break;
		case LEX_PACKET_CHECKSUM:
			state = LEX_WAITING;
			DIAGNOSTICS("Sum character from packet: %0x, actual sum of received bytes: %0x\n", c, 0xff-checksum);
			if (0xFF - checksum == c) {
				return ZB_VALID_PACKET;
			} else {
				return ZB_INVALID_PACKET;
			}
		default:
			break;
	}

	return ZB_PARSING;
}
