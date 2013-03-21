#include "zb_transport.h"
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include "diagnostics.h"
#define RX_BUFFER_SIZE 256
#define SERIAL_DEVICE "/dev/ttyAMA0"
#define SERIAL_BAUD_RATE 9600

/*
 * zb_transport_tty.h
 *
 * Seriel communication interface for tty device on the Raspberry Pi.
 *
 * Realised using pthreads for the receive buffer with a separate thread monitoring
 * the serial device.
 *
 *
 * Author: Kristian Hentschel
 * Team Project 3. University of Glasgow. 2013
 */

/* worker method. argument is a file descriptor for the serial interface */
static void *serial_monitor(void *arg);

typedef struct buffer {
	pthread_mutex_t lock;
	pthread_cond_t	nonfull;
	pthread_cond_t	nonempty;
	int count;
	int last;
	int first;
	char elements[RX_BUFFER_SIZE];
} Buffer;

/* global variables */
static pthread_t pthread_receiver;
static Buffer RX_buffer;
static int serial_fd;

/* open and setup the serial device.
 * initialise the buffer structure, locks, and condition variables.
 * start the monitoring thread
 */
void zb_transport_init() {
	struct termios tc;

	/* open serial port */
	serial_fd = open(SERIAL_DEVICE, O_RDWR | O_NOCTTY | O_NDELAY);
	if (serial_fd < 0) {
		printf("[CRITICAL] could not open serial device %s\n", SERIAL_DEVICE);
	}

	fcntl(serial_fd, F_SETFL, 0);
	
	/* set tc options for serial port transfers */
	tcgetattr(serial_fd, &tc);

	cfsetospeed(&tc, 9600);
	cfsetispeed(&tc, 9600);
	tc.c_cflag |= (CLOCAL | CREAD);
	tc.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);

	tcsetattr(serial_fd, TCSANOW, &tc);

	/* set up buffer structures and locks */
	pthread_mutex_init(&RX_buffer.lock, NULL);
	pthread_mutex_lock(&RX_buffer.lock);

	pthread_cond_init(&RX_buffer.nonfull, NULL);
	pthread_cond_init(&RX_buffer.nonempty, NULL);

	RX_buffer.count = 0;
	RX_buffer.last = 0;
	RX_buffer.first = 0;

	pthread_mutex_unlock(&RX_buffer.lock);

	/* start receiving thread to fill the buffer */	
	pthread_create(&pthread_receiver, NULL, serial_monitor, NULL); 
}

/* close serial device, destroy any threads and locks (TODO do it properly) */
void zb_transport_stop() {
	pthread_cancel(pthread_receiver);
	pthread_join(pthread_receiver, NULL);
	close(serial_fd);
}

void zb_send(unsigned char *buf, unsigned char len) {
	write(serial_fd, buf, len);
	fsync(serial_fd);
}

/* take a character from the buffer if it's not empty
 * or block until a character is available.
 */
char zb_getc() {
	char c;

	pthread_mutex_lock(&RX_buffer.lock);

	while (RX_buffer.count == 0) {
		pthread_cond_wait(&RX_buffer.nonempty, &RX_buffer.lock);
	}

	c = RX_buffer.elements[RX_buffer.first];
	RX_buffer.count--;
	RX_buffer.first = (RX_buffer.first + 1) % RX_BUFFER_SIZE;
	
	/* DIAGNOSTICS("getc: got %02x, %d in RX buffer\n", c, RX_buffer.count); */

	pthread_cond_signal(&RX_buffer.nonfull);
	pthread_mutex_unlock(&RX_buffer.lock);

	return c;
}

/* pause current thread for 1 second */
void zb_guard_delay() {
	/* TODO may want to use nanosleep() for POSIX conformity */
	usleep(1000 * 1000);
}

/* worker thread for monitoring serial device and putting stuff into buffer */
static void *serial_monitor(void *arg) {
	char c;

	printf("starting to read\n");
	while (read(serial_fd, &c, 1) > 0) {
		pthread_mutex_lock(&RX_buffer.lock);
		while (RX_buffer.count == RX_BUFFER_SIZE) {
			pthread_cond_wait(&RX_buffer.nonfull, &RX_buffer.lock);
		}
		
		RX_buffer.elements[RX_buffer.last] = c;
		RX_buffer.last = (RX_buffer.last + 1) % RX_BUFFER_SIZE;
		
		RX_buffer.count++;
		
		/* DIAGNOSTICS("read: got %02x, %d in RX buffer now.\n", c, RX_buffer.count); */

		pthread_cond_signal(&RX_buffer.nonempty);
		pthread_mutex_unlock(&RX_buffer.lock);
	}

	printf("[CRITICAL] read from serial device failed.\n");
	return NULL;
}
