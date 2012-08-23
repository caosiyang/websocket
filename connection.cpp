#include "connection.h"


//create a websocket connection
ws_conn_t *ws_conn_new() {
	ws_conn_t *conn = new (nothrow) ws_conn_t;
	if (conn) {
		conn->bev = NULL;
		conn->ws_req_str = "";
		conn->ws_resp_str = "";
		conn->step = ZERO;
		conn->ntoread = 0;
		conn->frame = frame_new();
		conn->handshake_cb_unit.cb = NULL;
		conn->handshake_cb_unit.cbarg = NULL;
		conn->frame_recv_cb_unit.cb = NULL;
		conn->frame_recv_cb_unit.cbarg = NULL;
		conn->write_cb_unit.cb = NULL;
		conn->write_cb_unit.cbarg = NULL;
		conn->close_cb_unit.cb = NULL;
		conn->close_cb_unit.cbarg = NULL;
		conn->ping_cb_unit.cb = NULL;
		conn->ping_cb_unit.cbarg = NULL;
	}
	return conn;
}


//destroy a websocket connection
void ws_conn_free(ws_conn_t *conn) {
	if (conn) {
		if (conn->frame) {
			frame_free(conn->frame);
			conn->frame = NULL;
		}
		delete conn;
	}
}


//websocket serve start
void ws_serve_start(ws_conn_t *conn) {
	if (conn && conn->bev) {
		accept_websocket_request(conn);
	} else {
		ws_serve_exit(conn);
	}
}


//websocket serve exit
void ws_serve_exit(ws_conn_t *conn) {
	if (conn) {
		if (conn->close_cb_unit.cb) {
			websocket_cb cb = conn->close_cb_unit.cb;
			void *cbarg = conn->close_cb_unit.cbarg;
			cb(cbarg);
		}
	}
}


//accept the websocket request
void accept_websocket_request(ws_conn_t *conn) {
	if (conn && conn->bev) {
		//read websocket request
		bufferevent_setcb(conn->bev, request_read_cb, response_write_cb, close_cb, conn); 
		bufferevent_setwatermark(conn->bev, EV_READ, 1, 1);
		bufferevent_setwatermark(conn->bev, EV_WRITE, 0, 0);
		bufferevent_enable(conn->bev, EV_READ);
	} else {
		ws_serve_exit(conn);
	}
}


//respond the websocket request
void respond_websocket_request(ws_conn_t *conn) {
	if (conn && conn->bev) {
		ws_req_t ws_req;
		parse_websocket_request(conn->ws_req_str.c_str(), &ws_req); //parse request
		//TODO
		//check if it is a websocket request
		//if (!valid) {
		//	ws_serve_exit(conn);
		//	return;
		//}
		conn->ws_resp_str = generate_websocket_response(&ws_req); //generate response
		if (!conn->ws_resp_str.empty()) {
			bufferevent_write(conn->bev, conn->ws_resp_str.c_str(), conn->ws_resp_str.length());
		} else {
			ws_serve_exit(conn);
		}
	} else {
		ws_serve_exit(conn);
	}
}


//request read callback
void request_read_cb(struct bufferevent *bev, void *ctx) {
	ws_conn_t *conn = (ws_conn_t*)ctx;
	if (conn && conn->bev) {
		char c;
		bufferevent_read(bev, &c, 1);
		conn->ws_req_str += c;
		size_t n = conn->ws_req_str.size();
		//TODO
		//for security
		//if (n > MAX_WS_REQ_LEN) {
		//	ws_serve_exit();
		//}

		//receive request completely
		if (n >= 4 && conn->ws_req_str.substr(n - 4) == "\r\n\r\n") {
			bufferevent_disable(conn->bev, EV_READ); //stop reading before a valid handshake
			respond_websocket_request(conn); //send websocket response
		}
	} else {
		ws_serve_exit(conn);
	}
}


//response write callback
void response_write_cb(struct bufferevent *bev, void *ctx) {
	ws_conn_t *conn = (ws_conn_t*)ctx;
	if (conn && conn->bev) {
		if (conn->handshake_cb_unit.cb) {
			websocket_cb cb = conn->handshake_cb_unit.cb;
			void *cbarg = conn->handshake_cb_unit.cbarg;
			cb(cbarg);
		}
		LOG("%s", conn->ws_req_str.c_str());
		LOG("%s", conn->ws_resp_str.c_str());

		frame_recv_loop(conn); //frame receive loop
	} else {
		ws_serve_exit(conn);
	}
}


//send a frame
inline int32_t send_a_frame(ws_conn_t *conn, const frame_buffer_t *fb) {
	return bufferevent_write(conn->bev, fb->data, fb->len) == fb->len ? 0 : -1;
}


void frame_recv_loop(ws_conn_t *conn) {
	if (conn && conn->bev) {
		conn->step = ONE;
		conn->ntoread = 2;
		bufferevent_setcb(conn->bev, frame_read_cb, write_cb, close_cb, conn);
		bufferevent_setwatermark(conn->bev, EV_READ, conn->ntoread, conn->ntoread);
		bufferevent_setwatermark(conn->bev, EV_WRITE, 0, 0);
		bufferevent_enable(conn->bev, EV_READ);
	} else {
		ws_serve_exit(conn);
	}
}


void frame_read_cb(struct bufferevent *bev, void *ctx) {
	ws_conn_t *conn = (ws_conn_t*)ctx;
	if (!conn || !conn->bev) {
		ws_serve_exit(conn);
		return;
	}

	switch (conn->step) {
	case ONE:
		{
			LOG("---- STEP 1 ----");
			char tmp[conn->ntoread];
			bufferevent_read(bev, tmp, conn->ntoread);
			//parse header
			if (parse_frame_header(tmp, conn->frame) == 0) {
				LOG("FIN         = %lu", conn->frame->fin);
				LOG("OPCODE      = %lu", conn->frame->opcode);
				LOG("MASK        = %lu", conn->frame->mask);
				LOG("PAYLOAD_LEN = %lu", conn->frame->payload_len);
				//payload_len is [0, 127]
				if (conn->frame->payload_len <= 125) {
					conn->step = THREE;
					conn->ntoread = 4;
					bufferevent_setwatermark(bev, EV_READ, conn->ntoread, conn->ntoread);
				} else if (conn->frame->payload_len == 126) {
					conn->step = TWO;
					conn->ntoread = 2;
					bufferevent_setwatermark(bev, EV_READ, conn->ntoread, conn->ntoread);
				} else if (conn->frame->payload_len == 127) {
					conn->step = TWO;
					conn->ntoread = 8;
					bufferevent_setwatermark(bev, EV_READ, conn->ntoread, conn->ntoread);
				}
			}
			//TODO
			//validate frame header
			if (!is_frame_valid(conn->frame)) {
				return;
			}
			break;
		}

	case TWO:
		{
			LOG("---- STEP 2 ----");
			char tmp[conn->ntoread];
			bufferevent_read(bev, tmp, conn->ntoread);
			if (conn->frame->payload_len == 126) {
				conn->frame->payload_len = ntohs(*(uint16_t*)tmp);
				LOG("PAYLOAD_LEN = %lu", conn->frame->payload_len);
			} else if (conn->frame->payload_len == 127) {
				conn->frame->payload_len = myntohll(*(uint64_t*)tmp);
				LOG("PAYLOAD_LEN = %llu", conn->frame->payload_len);
			}
			conn->step = THREE;
			conn->ntoread = 4;
			bufferevent_setwatermark(bev, EV_READ, conn->ntoread, conn->ntoread);
			break;
		}

	case THREE:
		{
			LOG("---- STEP 3 ----");
			char tmp[conn->ntoread];
			bufferevent_read(bev, tmp, conn->ntoread);
			memcpy(conn->frame->masking_key, tmp, conn->ntoread);
			if (conn->frame->payload_len > 0) {
				conn->step = FOUR;
				conn->ntoread = conn->frame->payload_len;
				bufferevent_setwatermark(bev, EV_READ, conn->ntoread, conn->ntoread);
			} else if (conn->frame->payload_len == 0) {
				/*recv a whole frame*/
				if (conn->frame->mask == 0) {
					//recv an unmasked frame
				}
				if (conn->frame->fin == 1 && conn->frame->opcode == 0x8) {
					//0x8 denotes a connection close
					frame_buffer_t *fb = frame_buffer_new(1, 8, 0, NULL);
					send_a_frame(conn, fb);
					LOG("send a close frame");
					frame_buffer_free(fb);

#if 0
					if (conn->conn_close_cb_unit.cb) {
						websocket_cb cb = conn->conn_close_cb_unit.cb;
						void *cbarg = conn->conn_close_cb_unit.cbarg;
						cb(cbarg);
					} else {
						//bufferevent_disable(conn->bev, EV_READ | EV_WRITE);
					}
#endif
					break;
				} else if (conn->frame->fin == 1 && conn->frame->opcode == 0x9) {
					//0x9 denotes a ping
					//TODO
					//make a pong
				} else {
					//execute custom operation
					if (conn->frame_recv_cb_unit.cb) {
						websocket_cb cb = conn->frame_recv_cb_unit.cb;
						void *cbarg = conn->frame_recv_cb_unit.cbarg;
						cb(cbarg);
					}
				}

				conn->step = ONE;
				conn->ntoread = 2;
				bufferevent_setwatermark(bev, EV_READ, conn->ntoread, conn->ntoread);
			}
			break;
		}

	case FOUR:
		{
			LOG("---- STEP 4 ----");
			if (conn->frame->payload_len > 0) {
				if (conn->frame->payload_data) {
					delete[] conn->frame->payload_data;
					conn->frame->payload_data = NULL;
				}
				conn->frame->payload_data = new char[conn->frame->payload_len];
				bufferevent_read(bev, conn->frame->payload_data, conn->frame->payload_len);
				unmask_payload_data(conn->frame);
			}


			/*recv a whole frame*/
			if (conn->frame->fin == 1 && conn->frame->opcode == 0x8) {
				//0x8 denotes a connection close
				frame_buffer_t *fb = frame_buffer_new(1, 8, 0, NULL);
				send_a_frame(conn, fb);
				LOG("send a close frame");
				frame_buffer_free(fb);

#if 0
				if (conn->conn_close_cb_unit.cb) {
					websocket_cb cb = conn->conn_close_cb_unit.cb;
					void *cbarg = conn->conn_close_cb_unit.cbarg;
					cb(cbarg);
				} else {
					//bufferevent_disable(conn->bev, EV_READ | EV_WRITE);
				}
#endif
				break;
			} else if (conn->frame->fin == 1 && conn->frame->opcode == 0x9) {
				//0x9 denotes a ping
				//TODO
				//make a pong
			} else {
				//execute custom operation
				if (conn->frame_recv_cb_unit.cb) {
					websocket_cb cb = conn->frame_recv_cb_unit.cb;
					void *cbarg = conn->frame_recv_cb_unit.cbarg;
					cb(cbarg);
				}
			}


			if (conn->frame->opcode == 0x1) { //0x1 denotes a text frame
			}
			if (conn->frame->opcode == 0x2) { //0x1 denotes a binary frame
			}


			conn->step = ONE;
			conn->ntoread = 2;
			bufferevent_setwatermark(bev, EV_READ, conn->ntoread, conn->ntoread);
			break;
		}

	default:
		LOG("---- STEP UNKNOWN ----");
		LOG("exit");
		exit(-1);
		break;
	}

}


void ws_conn_setcb(ws_conn_t *conn, enum CBTYPE cbtype, websocket_cb cb, void *cbarg) {
	if (conn) {
		switch (cbtype) {
		case HANDSHAKE:
			conn->handshake_cb_unit.cb = cb;
			conn->handshake_cb_unit.cbarg = cbarg;
			break;
		case FRAME_RECV:
			conn->frame_recv_cb_unit.cb = cb;
			conn->frame_recv_cb_unit.cbarg = cbarg;
			break;
		case WRITE:
			conn->write_cb_unit.cb = cb;
			conn->write_cb_unit.cbarg = cbarg;
			break;
		case CLOSE:
			conn->close_cb_unit.cb = cb;
			conn->close_cb_unit.cbarg = cbarg;
			break;
		case PING:
			conn->ping_cb_unit.cb = cb;
			conn->ping_cb_unit.cbarg = cbarg;
			break;
		default:
			break;
		}
	}
}


void write_cb(struct bufferevent *bev, void *ctx) {
	ws_conn_t *conn = (ws_conn_t*)ctx;
	if (conn) {
		if (conn->write_cb_unit.cb) {
			websocket_cb cb = conn->write_cb_unit.cb;
			void *cbarg = conn->write_cb_unit.cbarg;
			cb(cbarg);
		}
	}
}


void close_cb(struct bufferevent *bev, short what, void *ctx) {
	ws_serve_exit((ws_conn_t*)ctx);
}
