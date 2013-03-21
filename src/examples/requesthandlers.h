#ifndef __MASTER_REQUESTHANDLERS_H__
#define __MASTER_REQUESTHANDLERS_H__

/*
 * master_requesthandlers.h
 *
 * request handlers serialise client requests received from the mock application
 * or through http requests from the client. diagnostics are printed to stdout.
 * JSON formatted data will be returned in the buffers provided.
 *
 * through the request handlers, application state is managed as they
 * implement the application layer of the system. No other components should make calls to
 * the underlying transport layer after the network device has been set up
 *
 * Author: Kristian Hentschel
 * Team Project 3. University of Glasgow
 */


/* all request methods will store the result to be sent to the client in a buffer
 * that must be of this size or bigger. */
#define REQUEST_RESULT_BUFSIZE 4096

#define SENSOR_COUNT 5
typedef unsigned long sensor_data_t;

void REQUEST_measure(char *buf);
void REQUEST_calibrate(char *buf);
void REQUEST_data(char *buf);
void REQUEST_ping(char *buf);

void HANDLE_packet_received();
#endif /*__MASTER_REQUESTHANDLERS_H__*/

