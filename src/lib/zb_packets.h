#ifndef __ZB_PACKETS_H__
#define __ZB_PACKETS_H__

/*
 * zb_packets.h
 *
 * Author: Kristian Hentschel
 * Team Project 3. University of Glasgow. 2013
 *
 * simple packet generation/parsing tool set for transmission over zigbee nodes.
 *
 * packet mechanism completely unrelated to zigbee implementation.
 *
 * Zigbees used in transparent (AT) mode.
 *
 */

#define MAX_PACKET_SIZE 72

#define OP_PING 0x00
#define OP_PONG 0x01
#define OP_MEASURE_REQUEST 0x10
#define OP_MEASURE_RESPONSE 0x20
/* global variables to hold parser results */
extern char zb_word_data[MAX_PACKET_SIZE];
extern int zb_word_len;

extern char zb_packet_data[MAX_PACKET_SIZE];
extern char zb_packet_op;
extern char zb_packet_from;
extern char zb_packet_len;

/* return type of parser function */
enum zb_parse_response {
	ZB_PARSING,
	ZB_PLAIN_WORD,
	ZB_START_PACKET,
	ZB_VALID_PACKET,
	ZB_INVALID_PACKET
};

/*
 * Initialises the packet system state and lower level components.
 * Ensures that UART transfer with escape characters is enabled.
 */
void zb_packets_init();

/* 
 * sets the target address for transmissions sent by this device.
 * the packet layer currently only supports addressing all devices on the network (broadcast = 1)
 * or only the coordinator (unicast, broadcast = 0)
 */
void zb_set_broadcast_mode(char broadcast);

/*
 * sets the application level device identifier for this device. This should be in the range of 1-4 for sensors, and 0 for the master unit.
 * This has no relation to the network or hardware (MAC) address of the radio unit.
 */
void zb_set_device_id(char id);

/* sends an AT command to the radio unit for reading or setting configuration parameters. */
void zb_send_command_with_argument(char cmd[2], char *data, unsigned char len);
void zb_send_command(char cmd[2]);

/* packetizes the data and sends it to the radio unit via the transport layer. */
void zb_send_packet(char type, unsigned char *data, unsigned char len);

/* 
 * parses the response, should be called in order on every character received.
 * only guarantees that the data stored in global variables is valid between returning
 * ZB_VALID_PACKET and the next call to this method.
 *
 * Return values:
 *  - ZB_PARSING - no valid response yet.
 *  - ZB_PLAIN_WORD - not a valid packet, but an alphanumeric word separated by spaces or line endings.
 *  	Result will be valid in zb_word_data and zb_word_len global variables.
 *  - ZB_VALID_PACKET - A complete valid packet, matching the checksum and length fields, has been received.
 *  	Result will be valid in zb_packet_data, zb_packet_from, and zb_packet_len.
 *  - ZB_INVALID_PACKET - a packet with unknown API frame type or invalid checksum has been received.
 */
enum zb_parse_response zb_parse(unsigned char c);

#endif /* __ZB_PACKETS_H__ */
