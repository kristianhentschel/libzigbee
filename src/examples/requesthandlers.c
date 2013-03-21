#include "requesthandlers.h"
#include "zb_transport.h"
#include "zb_packets.h"
#include "diagnostics.h"
#include <pthread.h>
#include <time.h>
#include <ctype.h>
#include <string.h>

#define TIMEOUT 1

/* this implementation of the REQUEST functions is not thread-safe. only one thread should be calling them. */

/* private types */
struct sensor_result {
	int device_id;
	sensor_data_t data;
	time_t time;
	char is_valid;
	pthread_mutex_t lock;
};

struct sensor_config {
	int device_id;
	sensor_data_t offset;
	time_t calibrated;
};

static struct sensor_config sensor_configs[SENSOR_COUNT];
static struct sensor_result sensor_results[SENSOR_COUNT];

enum comms_state {STATE_IDLE, STATE_PENDING_MEASURE, STATE_PENDING_CALIBRATE};

/* private variables */
static enum comms_state state;
time_t last_request_time = 0;

/* static methods */
static unsigned int hexToInt(char *buf, unsigned char len);
static double convert_sensor_value(double value);

/* check if the system is still waiting on previous responses to occur within TIMEOUT of last transmission. */
static int busy() {
	if (time(NULL) - last_request_time > TIMEOUT) {
		state = STATE_IDLE;
	}
	return state != STATE_IDLE;
}

void sensors_init() {
	int i;
	
	state = STATE_IDLE;

	for (i = 0; i < SENSOR_COUNT; i++) {
		pthread_mutex_init(&sensor_results[i].lock, NULL);
		pthread_mutex_lock(&sensor_results[i].lock);
		sensor_results[i].device_id = i;
		sensor_results[i].data = 0;
		sensor_results[i].time = 0;
		sensor_results[i].is_valid = 0;
		pthread_mutex_unlock(&sensor_results[i].lock);
		
		sensor_configs[i].device_id = i;
		sensor_configs[i].offset = 0; /* TODO get from config file or something? */
		sensor_configs[i].calibrated = 0;
	}
}


void REQUEST_measure(char *buf) {
	if (!busy()) {
		DIAGNOSTICS("MEASURE: sending broadcast message to get measurements\n");
		state = STATE_PENDING_MEASURE;
		last_request_time = time(NULL);
		zb_send_packet(OP_MEASURE_REQUEST, NULL, 0);
		sprintf(buf, "200 OK Measurement requested.\n");
	} else {
		DIAGNOSTICS("MEASURE:  request not honoured as the system is currently busy.\n");
		sprintf(buf, "300 BUSY Measurement not requested as previous requests are still pending.\n");
	}
}

/*
 * calibration requested by user. gets raw data from sensors, and sets this value as the zero point for that sensor.
 * system will wait for all sensors to successfully return data before allowing new measurements.
 */
void REQUEST_calibrate(char *buf) {
	int i;

	if (!busy()) {
		DIAGNOSTICS("CALIBRATE: sending broadcast message to get raw measurements\n");
		for (i = 1; i < SENSOR_COUNT; i++) {
			sensor_configs[i].calibrated = 0;
		}
		state = STATE_PENDING_CALIBRATE;
		last_request_time = time(NULL);
		zb_send_packet(OP_MEASURE_REQUEST, NULL, 0);
		sprintf(buf, "200 OK Calibration requested.\n");
	} else {
		DIAGNOSTICS("CALIBRATE: request not honoured as the system is currently busy.\n");
		sprintf(buf, "300 BUSY Calibration not requested as previous requests are still pending.\n");
	}
}

void REQUEST_ping(char *buf) {
	if (!busy()) {
		DIAGNOSTICS("PING sent.\n");
		state = STATE_PENDING_CALIBRATE;
		zb_send_packet(OP_PING, NULL, 0);
		last_request_time = time(NULL);
		sprintf(buf, "200 OK Ping request sent.\n");
	} else {
		DIAGNOSTICS("PING request not honoured as the system is currently busy.\n");
		sprintf(buf, "300 BUSY ping: previous requests still pending.\n");
	}
	buf[0] = '\0';
}

void REQUEST_data(char *buf) {
	int i;
	char internalbuf[REQUEST_RESULT_BUFSIZE];
	DIAGNOSTICS("DATA: Returning current sensor data.\n");

	buf[0] = '\0';
	snprintf(internalbuf, REQUEST_RESULT_BUFSIZE,
			"{\"sensors\": [");
	strcat(buf, internalbuf);

	for (i = 0; i < SENSOR_COUNT; i++) {
		pthread_mutex_lock(&sensor_results[i].lock);

		snprintf(internalbuf, REQUEST_RESULT_BUFSIZE,
				"{\"value\": %d, \"converted\": %.2f, \"time\": %d, \"offset\": %d}",
				(int) sensor_results[i].data - sensor_configs[i].offset,
				convert_sensor_value((double)sensor_results[i].data - (double)sensor_configs[i].offset),
				(int) sensor_results[i].time,
				(int) sensor_configs[i].offset);
		strcat(buf, internalbuf);

		if (i < SENSOR_COUNT - 1) {
			strcat(buf, ",\n");
		}

		DIAGNOSTICS("DATA: Sensor %d: Raw %d, Offset %d, Corrected %d. Last read at %d\n",
				i,
				(int) sensor_results[i].data,
				(int) sensor_configs[i].offset,
				(int) (sensor_results[i].data - sensor_configs[i].offset),
				(int) sensor_results[i].time);

		pthread_mutex_unlock(&sensor_results[i].lock);
	}
	strcat(buf, "]}");
}



void HANDLE_packet_received() {
	int d, i;
	switch (zb_packet_op) {
		case OP_PING:
			DIAGNOSTICS("Received PING request from %d.\n", zb_packet_from);
			zb_send_packet(OP_PONG, NULL, 0);
			break;
		case OP_PONG:
			DIAGNOSTICS("Received PONG from %d.\n", zb_packet_from);
			break;
		case OP_MEASURE_REQUEST:
			DIAGNOSTICS("Received measure request. Ignoring on master unit.\n");
			break;
		case OP_MEASURE_RESPONSE:
			DIAGNOSTICS("Received measure response from %d. Updating sensor result.\n", zb_packet_from);
			if (zb_packet_from >= SENSOR_COUNT || zb_packet_from == 0) {
				DIAGNOSTICS("response from unknown sensor. ignoring.\n");
				break;
			}

			d = zb_packet_from;

			pthread_mutex_lock(&sensor_results[d].lock);
			sensor_results[d].data = hexToInt(zb_packet_data, zb_packet_len);
			sensor_results[d].time = time(NULL); /* TODO gettimeofday for more resolution? */

			if (state == STATE_PENDING_CALIBRATE) {
				sensor_configs[d].offset = sensor_results[d].data;
				sensor_configs[d].calibrated = 1;

				state = STATE_IDLE;

				for (i = 1; i < SENSOR_COUNT; i++) {
					if (sensor_configs[i].calibrated == 0) {
						state = STATE_PENDING_CALIBRATE;
					}
				}
			}

			pthread_mutex_unlock(&sensor_results[d].lock);
			break;
		default:
			DIAGNOSTICS("Received packet with unsupported OP-code. ignoring.\n");
	}
}

/* convert a string of hexadecimal numbers to an integer */
static unsigned int hexToInt(char *buf, unsigned char len) {
	int i;
	char c;
	unsigned int result, v;

	result = 0;
	for (i = 0; i < len; i++) {
		c = tolower((int) buf[i]);
		if (c >= 'a' && c <= 'f') {
			v = c - 'a' + 10;
		} else if (c >= '0' && c <= '9') {
			v = c - '0';
		} else {
			v = 0;
		}
		result = result * 16 + v;
	}
	return result;
}

/* convert a data value received from a sensor to a weight value in kilogram
 * TODO this is a mock calculation as actual data for the real load cells and
 * strain gauge configurations is not yet available.
 */
double convert_sensor_value(double value) {
	return ((double) value) / 41;
}
