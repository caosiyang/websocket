/*
 *
 * file:  webchat.h
 * author: caosiyang
 * email: csy3228@gmail.com
 *
 */
#ifndef __WEBCHAT_H__
#define __WEBCHAT_H__

#include "websocket.h"
#include "webchat.h"
#include "event2/event.h"
#include "event2/listener.h"
#include "event2/bufferevent.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <assert.h>
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <map>
using namespace std;


//frame step
enum Step {
	ZERO,
	ONE, //0-2 bytes, fin, opcode, mask, payload len
	TWO, //extended payload length
	THREE //masking-key and payload-data
	//FOUR //payload data
};


typedef struct user {
	struct bufferevent *bev;
	string ws_req_str;
	enum Step step;
	int32_t ntoread;
	message_frame_header_t message_frame_header;
	string content;
} user_t;


#endif
