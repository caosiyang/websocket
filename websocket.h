/*
 *
 * file:   websocket.h
 * author: caosiyang
 * email:  csy3228@gmail.com
 * date:   2012/04/23
 *
 */
#ifndef __WEBSOCKET_H__
#define __WEBSOCKET_H__

#include "openssl/sha.h"
#include "base64.h"
#include <iostream>
#include <string>
using namespace std;


//#define myhtons(n)  ((((uint16_t)(n) & 0xff00) >> 8) | (((uint16_t)(n) & 0x00ff) << 8))


typedef struct websocket_request {
	string req;
	string connection;
	string upgrade;
	string host;
	string origin;
	string cookie;
	string sec_websocket_key;
	string sec_websocket_version;
} ws_req_t;


typedef struct websocket_response {
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


typedef struct message_frame_header {
	uint8_t fin;
	uint8_t opcode;
	uint8_t mask;
	uint64_t payload_len;
	unsigned char masking_key[4];
} message_frame_header_t;


inline uint16_t myhtons(uint16_t n) {
	return ((n & 0xff00) >> 8) | ((n & 0x00ff) << 8);
}


inline uint16_t myntohs(uint16_t n) {
	return ((n & 0xff00) >> 8) | ((n & 0x00ff) << 8);
}


inline uint32_t myhtonl(uint32_t n) {
	return ((n & 0xff000000) >> 24) | ((n & 0x00ff0000) >> 8) | ((n & 0x0000ff00) << 8) | ((n & 0x000000ff) << 24);
}


inline uint32_t myntohl(uint32_t n) {
	return ((n & 0xff000000) >> 24) | ((n & 0x00ff0000) >> 8) | ((n & 0x0000ff00) << 8) | ((n & 0x000000ff) << 24);
}


inline uint64_t myhtonll(uint64_t n) {
	return (uint64_t)myhtonl(n >> 32) | ((uint64_t)myhtonl(n) << 32);
}


inline uint64_t myntohll(uint64_t n) {
	return (uint64_t)myhtonl(n >> 32) | ((uint64_t)myhtonl(n) << 32);
}


int32_t parse_websocket_request(const char *src, ws_req_t *ws_req);


void print_websocket_request(const ws_req_t *ws_req);


static string generate_key(const string &key);


string generate_websocket_response(const ws_req_t *ws_req);


//parse fin, opcode, mask, payload len
int32_t read_fin_opcode_mask_payloadlen(const char *buf, message_frame_header_t *header);


int32_t unmask_payload_data(const char *masking_key, char *payload_data, uint32_t payload_len);


int32_t generate_message_frame(const char *payload_data, uint32_t payload_len, char **message_frame, uint32_t *message_frame_len);


#endif
