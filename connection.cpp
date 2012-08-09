#include "connection.h"


//create a websocket connection
ws_conn_t *ws_conn_create() {
	ws_conn_t *conn = new (nothrow) ws_conn_t;
	if (conn) {
		conn->bev = NULL;
		conn->ws_req_str = "";
		conn->step = ZERO;
		conn->ntoread = 0;
		conn->frame = frame_new();
		conn->handshake_cb_unit.cb = NULL;
		conn->handshake_cb_unit.cbarg = NULL;
		conn->frame_send_cb_unit.cb = NULL;
		conn->frame_send_cb_unit.cb = NULL;
		conn->frame_recv_cb_unit.cb = NULL;
		conn->frame_recv_cb_unit.cbarg = NULL;
		conn->conn_close_cb_unit.cbarg = NULL;
		conn->conn_close_cb_unit.cbarg = NULL;
	}
	return conn;
}


//destroy a websocket connection
void ws_conn_destroy(ws_conn_t *conn) {
	if (conn) {
		if (conn->bev) {
			bufferevent_free(conn->bev);
		}
		if (conn->frame) {
			frame_free(conn->frame);
		}
		delete conn;
	}
}


//read the websocket request
void accept_websocket_request(ws_conn_t *conn) {
	LOG("%s", __func__);
	if (conn && conn->bev) {
		//bufferevent_getcb(conn->bev, &(conn->readcb), &(conn->writecb), &(conn->eventcb), &(conn->cbarg));
		bufferevent_setcb(conn->bev, request_read_cb, NULL, conn_close_cb, conn); 
		bufferevent_setwatermark(conn->bev, EV_READ, 1, 1);
		bufferevent_enable(conn->bev, EV_READ | EV_WRITE);
	}
}


void respond_websocket_request(ws_conn_t *conn) {
	//handshake
	ws_req_t ws_req;
	parse_websocket_request(conn->ws_req_str.c_str(), &ws_req);
	string resp = generate_websocket_response(&ws_req);
	bufferevent_write(conn->bev, resp.c_str(), resp.length());
	bufferevent_disable(conn->bev, EV_READ);
	LOG("%s", conn->ws_req_str.c_str());
	LOG("%s", resp.c_str());
}


void request_read_cb(struct bufferevent *bev, void *ctx) {
	ws_conn_t *conn = (ws_conn_t*)ctx;
	char c;
	bufferevent_read(bev, &c, 1);
	conn->ws_req_str += c;
	//check if receive completely
	size_t n = conn->ws_req_str.size();
	//for security
	//if (n > MAX_WS_REQ_LEN) {
	//}
	//check if receive request completely
	if (n >= 4 && conn->ws_req_str.substr(n - 4) == "\r\n\r\n") {
		if (conn->handshake_cb_unit.cb) {
			LOG("handshake_cb != NULL");
			websocket_cb cb = conn->handshake_cb_unit.cb;
			void *cbarg = conn->handshake_cb_unit.cbarg;
			cb(cbarg);
		} else {
			LOG("handshake_cb == NULL");
			respond_websocket_request(conn);
		}

		//frame recv loop
		frame_recv_loop(conn);
	}
}


//send a frame
int32_t send_a_frame(ws_conn_t *conn, const frame_buffer_t *fb) {
	if (bufferevent_write(conn->bev, fb->data, fb->len) == fb->len) {
		return 0;
	}
	return -1;

}


void frame_recv_loop(ws_conn_t *conn) {
	conn->step = ONE;
	conn->ntoread = 2;
	bufferevent_setcb(conn->bev, frame_read_cb, NULL, conn_close_cb, conn);
	bufferevent_setwatermark(conn->bev, EV_READ, conn->ntoread, conn->ntoread);
	bufferevent_enable(conn->bev, EV_READ);
}


void frame_read_cb(struct bufferevent *bev, void *ctx) {
	//LOG("%s", __func__);
	ws_conn_t *conn = (ws_conn_t*)ctx;

	switch (conn->step) {
	case ONE:
		{
			LOG("---- STEP 1 ----");
			char tmp[conn->ntoread];
			bufferevent_read(bev, tmp, conn->ntoread);
			//LOG("i read %d bytes", conn->ntoread);
			//parse header
			if (parse_frame_header(tmp, conn->frame) == 0) {
				LOG("FIN         = %d", conn->frame->fin);
				LOG("OPCODE      = %d", conn->frame->opcode);
				LOG("MASK        = %d", conn->frame->mask);
				LOG("PAYLOAD_LEN = %d", conn->frame->payload_len);
			}

			//payload_len is [0, 127]
			if (conn->frame->payload_len <= 125) {
				conn->step = THREE;
				conn->ntoread = 4;
				bufferevent_setwatermark(bev, EV_READ, conn->ntoread, conn->ntoread);
			}
			else if (conn->frame->payload_len == 126) {
				conn->step = TWO;
				conn->ntoread = 2;
				bufferevent_setwatermark(bev, EV_READ, conn->ntoread, conn->ntoread);
			}
			else if (conn->frame->payload_len == 127) {
				conn->step = TWO;
				conn->ntoread = 8;
				bufferevent_setwatermark(bev, EV_READ, conn->ntoread, conn->ntoread);
			}
			break;
		}

	case TWO:
		{
			LOG("---- STEP 2 ----");
			char tmp[conn->ntoread];
			bufferevent_read(bev, tmp, conn->ntoread);
			//LOG("i read %d bytes", conn->ntoread);
			if (conn->frame->payload_len == 126) {
				conn->frame->payload_len = ntohs(*(uint16_t*)tmp);
				LOG("PAYLOAD_LEN = %d", conn->frame->payload_len);
			}
			if (conn->frame->payload_len == 127) {
				conn->frame->payload_len = myntohll(*(uint64_t*)tmp);
				LOG("PAYLOAD_LEN = %d", conn->frame->payload_len);
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
			}
			else if (conn->frame->payload_len == 0) {
				/*recv a whole frame*/
				if (conn->frame->mask == 0) {
					//recv an unmasked frame
				}
				if (conn->frame->fin == 1 && conn->frame->opcode == 0x8) {
					//0x8 denotes a connection close
					frame_t *f = frame_new();
					f->fin = 1;
					f->opcode = 8;
					f->mask = 0;
					f->payload_len = 0;
					frame_buffer_t *fb = generate_frame_buffer(f);
					send_a_frame(conn, fb);
					LOG("send a close frame");
					frame_buffer_free(fb);
					frame_free(f);

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
				frame_t *f = frame_new();
				f->fin = 1;
				f->opcode = 8;
				f->mask = 0;
				f->payload_len = 0;
				frame_buffer_t *fb = generate_frame_buffer(f);
				send_a_frame(conn, fb);
				LOG("send a close frame");
				frame_buffer_free(fb);
				frame_free(f);

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
		case FRAME_SEND:
			conn->frame_send_cb_unit.cb = cb;
			conn->frame_send_cb_unit.cbarg = cbarg;
			break;
		case FRAME_RECV:
			conn->frame_recv_cb_unit.cb = cb;
			conn->frame_recv_cb_unit.cbarg = cbarg;
			break;
#if 0
		case MESSAGE_SEND:
			conn->message_send_cb_unit.cb = cb;
			conn->message_send_cb_unit.cbarg = cbarg;
			break;
		case MESSAGE_RECV:
			conn->message_recv_cb_unit.cb = cb;
			conn->message_recv_cb_unit.cbarg = cbarg;
			break;
#endif
		case CONN_CLOSE:
			conn->conn_close_cb_unit.cb = cb;
			conn->conn_close_cb_unit.cbarg = cbarg;
			break;
		default:
			break;
		}
	}
}


void conn_close_cb(struct bufferevent *bev, short what, void *ctx) {
	ws_conn_t * conn = (ws_conn_t*)ctx;
	if (conn->conn_close_cb_unit.cb) {
		websocket_cb cb = conn->conn_close_cb_unit.cb;
		void *cbarg = conn->conn_close_cb_unit.cbarg;
		cb(cbarg);
	}
}
