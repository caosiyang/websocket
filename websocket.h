/**
 *
 * filename: websocket.h
 * summary:
 * author: caosiyang
 * email:  csy3228@gmail.com
 *
 */
#ifndef WEBSOCKET_H
#define WEBSOCKET_H

#include "stdio.h"
#include "string.h"
#include "tools.h"
#include "frame.h"
#include "openssl/sha.h"
#include "base64.h"
#include <iostream>
#include <string>
#include <vector>
using namespace std;


//WebSocket request
typedef struct WebsocketRequest {
	string req;
	string connection;
	string upgrade;
	string host;
	string origin;
	string cookie;
	string sec_websocket_key;
	string sec_websocket_version;
} ws_req_t;


//WebSocket response
typedef struct WebsocketResponse {
	string resp;
	string date;
	string connection;
	string server;
	string upgrade;
	string access_control_allow_origin;
	string access_control_allow_credentials;
	string sec_websocket_accept;
	string access_control_allow_headers;
} ws_resp_t;


//steps of receiving a frame
enum Step {
	ZERO,  //before websocket handshake
	ONE,   //0-2 bytes, fin, opcode, mask, payload length
	TWO,   //extended payload length
	THREE, //masking-key
	FOUR,  //payload data
	UNKNOWN
};


//parse websocket request
int32_t parse_websocket_request(const char *src, ws_req_t *ws_req);


//print websocket request
void print_websocket_request(const ws_req_t *ws_req);


//generate websocket response
string generate_websocket_response(const ws_req_t *ws_req);


//generate websocket key
string generate_key(const string &key);


//parse fin, opcode, mask, payload len
int32_t parse_frame_header(const char *buf, frame_t *frame);


//unmask payload-data
int32_t unmask_payload_data(frame_t *frame);
int32_t unmask_payload_data(const char *masking_key, char *payload_data, uint32_t payload_len);


#endif
