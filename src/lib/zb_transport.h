#ifndef __ZB_TRANSPORT_H__
#define __ZB_TRANSPORT_H__
/* 
 * zb_transport.h
 *
 * Author: Kristian Hentschel
 * Team Project 3. University of Glasgow. 2013.
 *
 * Abstraction for accessing a serial connection to a Zigbee device.
 * There will be several implementations:
 * 	- zb_transport_tty.c	Target: Raspberry Pi. Buffer managed and populated using pthreads library.
 * 	- zb_transport_embedded.c	Target: STM32F4/F0. Buffer managed and populated using interrupts and a USART peripheral.
 *
 */

/* opens the serial device and initialises any receive buffer structures. */
void zb_transport_init();

/* closes the serial device connection and destroys the buffers if required. */
void zb_transport_stop();

/* sends a complete data packet over the serial line */
void zb_send(unsigned char *buf, unsigned char len);

/* blocks until a character is available in the serial buffer */
char zb_getc();

/* blocks for (at least!) one second, used for guard timings on entering command mode */
void zb_guard_delay();


#endif /* __ZB_TRANSPORT_H__ */
