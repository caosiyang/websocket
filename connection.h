/**
 *
 * filename: connection.h
 * summary: websocket connection
 * author: caosiyang
 * email: csy3228@gmail.com
 *
 */
#ifndef CONNECTION_H
#define CONNECTION_H

#include "websocket.h"
#include <event2/event.h>
#include <event2/bufferevent.h>
#include <iostream>
using namespace std;


typedef void(*websocket_cb)(void*);


typedef struct {
	websocket_cb cb;
	void *cbarg;
} ws_cb_unit;


typedef struct websocket_connection {
	struct bufferevent *bev;
	string ws_req_str;
	string ws_resp_str;
	enum Step step;
	uint64_t ntoread;
	frame_t *frame; //current frame
	ws_cb_unit handshake_cb_unit;
	ws_cb_unit frame_recv_cb_unit;
	ws_cb_unit write_cb_unit;
	ws_cb_unit close_cb_unit;
	ws_cb_unit ping_cb_unit;
} ws_conn_t;


//callback type
enum CBTYPE {
	HANDSHAKE,
	FRAME_RECV,
	WRITE,
	CLOSE,
	PING
};


//
// following functions are for library-users
//
//create a websocket connection
ws_conn_t *ws_conn_new();


//destroy a websocket connection
void ws_conn_free(ws_conn_t *conn);


//set callback
//MUST set: frame_read_cb, write_cb, close_cb
void ws_conn_setcb(ws_conn_t *conn, enum CBTYPE cbtype, websocket_cb cb, void *cbarg);


//websocket serve start
void ws_serve_start(ws_conn_t *conn);


//websocket serve exit
void ws_serve_exit(ws_conn_t *conn);


//send a frame
int32_t send_a_frame(ws_conn_t *conn, const frame_buffer_t *fb);




//
// following functions are for internal
//
//accept the websocket request
void accept_websocket_request(ws_conn_t *conn);


//respond the websocket request
void respond_websocket_request(ws_conn_t *conn);


//receive a frame
void frame_recv_loop(ws_conn_t *conn);


//request read callback
void request_read_cb(struct bufferevent *bev, void *ctx);


//response write callback
void response_write_cb(struct bufferevent *bev, void *ctx);


//frame read callback
void frame_read_cb(struct bufferevent *bev, void *ctx);


//websocket write callback
void write_cb(struct bufferevent *bev, void *ctx);


//connection close callback
void close_cb(struct bufferevent *bev, short what, void *ctx);


#endif
