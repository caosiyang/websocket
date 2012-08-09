/**
 *
 * filename: connection.h
 * summary:
 * author: caosiyang
 *
 */
#ifndef CONNECTION_H
#define CONNECTION_H

#include "websocket.h"
#include <event2/event.h>
#include <event2/bufferevent.h>
#include <iostream>
using namespace std;


#define LOG(format, args...) fprintf(stdout, format"\n", ##args)


typedef void(*websocket_cb)(void*);
typedef void(*ws_cb)(void*);

typedef struct {
	ws_cb cb;
	void *cbarg;
} ws_cb_unit;


typedef struct websocket_connection {
	struct bufferevent *bev;
	string ws_req_str;
	enum Step step;
	uint32_t ntoread;

	frame_t *frame; //current frame

	ws_cb_unit handshake_cb_unit;
	ws_cb_unit frame_send_cb_unit;
	ws_cb_unit frame_recv_cb_unit;
	ws_cb_unit conn_close_cb_unit;
	//ws_cb_unit message_send_cb_unit;
	//ws_cb_unit message_recv_cb_unit;
} ws_conn_t;


enum CBTYPE {
	HANDSHAKE,
	FRAME_SEND,
	FRAME_RECV,
	CONN_CLOSE,
	MESSAGE_SEND,
	MESSAGE_RECV
};


//create a websocket connection
ws_conn_t *ws_conn_create();


//destroy a websocket connection
void ws_conn_destroy(ws_conn_t *conn);


//set callback
void ws_conn_setcb(ws_conn_t *conn, enum CBTYPE cbtype, websocket_cb cb, void *cbarg);


//accept the websocket request
void accept_websocket_request(ws_conn_t *conn);


//respond the websocket request
void respond_websocket_request(ws_conn_t *conn);


//send a message
//void send_a_message(ws_conn_t *conn);


//receive a message
//void recv_a_message(ws_conn_t *conn);


//send a frame
int32_t send_a_frame(ws_conn_t *conn, const frame_buffer_t *fb);


//receive a frame
void frame_recv_loop(ws_conn_t *conn);


//request read callback
void request_read_cb(struct bufferevent *bev, void *ctx);


//frame read callback
void frame_read_cb(struct bufferevent *bev, void *ctx);


//connection close callback
void conn_close_cb(struct bufferevent *bev, short what, void *ctx);


//read the websocket message
//void read_websocket_message(struct bufferevent *bev, void *cbarg);


#endif
