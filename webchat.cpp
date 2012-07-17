#include "webchat.h"


static struct event_base *base = NULL;
static evconnlistener *listener = NULL;
//static map<struct bufferevent*, user_t*> bev_user_map;
static vector<user_t*> user_vec;


static const int32_t WS_REQ_ONCE_READ = 1;
static const int32_t MAX_WS_REQ_LEN = 10240;
static const int32_t MAX_MSG_LEN = 1024;


#define LOG(format, args...) fprintf(stdout, format"\n", ##args)


static void close_conn(user_t *user) {
	LOG("%s", __func__);
	if (user) {
		if (user->bev) {
			bufferevent_setcb(user->bev, NULL, NULL, NULL, NULL);
			bufferevent_disable(user->bev, EV_READ | EV_WRITE);
			bufferevent_free(user->bev);
		}
		for (vector<user_t*>::iterator iter = user_vec.begin(); iter != user_vec.end(); ++iter) {
			if (*iter == user) {
				user_vec.erase(iter);
				break;
			}
		}
		delete user;
	}
	LOG("now, %d users connecting", user_vec.size());
}


#if 0
static user_t* get_user(struct bufferevent* bev) {
	map<struct bufferevent*, user_t*>::iterator iter = bev_user_map.find(bev);
	if (iter != bev_user_map.end()) {
		return iter->second;
	} else {
		return NULL;
	}
}
#endif


void read_message_data(struct bufferevent *bev, void *ctx) {
	//LOG("%s", __func__);
	user_t *user = (user_t*)ctx;
	LOG("\ni read %d bytes", user->ntoread);
	char tmp[user->ntoread];
	memset(tmp, 0, user->ntoread);
	bufferevent_read(bev, tmp, user->ntoread);

	//store the payload length of current message frame
	uint64_t payload_len = 0;

	switch (user->step) {
	case ZERO:
	case ONE:
		//read payload len
		if (read_fin_opcode_mask_payloadlen(tmp, &user->message_frame_header) == 0) {
			LOG("FIN = %d", user->message_frame_header.fin);
			LOG("OPCODE = %d", user->message_frame_header.opcode);
			LOG("MASK = %d", user->message_frame_header.mask);
			LOG("PAYLOAD_LEN = %d", user->message_frame_header.payload_len);

			if (user->message_frame_header.mask != 1) {
				close_conn(user);
				return;
			}
			if (user->message_frame_header.payload_len > MAX_MSG_LEN) {
				close_conn(user);
				return;
			}
#if 0
			if (user->message_frame_header.opcode = 0x8) {
				LOG("i recv a close opcode");
			}
#endif
		} else {
			close_conn(user);
			return;
		}

		//now payload_len is [0, 127]
		payload_len = user->message_frame_header.payload_len;
		if (payload_len >= 0 && payload_len <= 125) {
			if (payload_len > MAX_MSG_LEN || user->content.size() + payload_len > MAX_MSG_LEN) {
				close_conn(user);
				return;
			}
			user->step = THREE;
			user->ntoread = 4 + payload_len;
			bufferevent_setwatermark(bev, EV_READ, user->ntoread, user->ntoread);
		}
		if (payload_len == 126) {
			user->step = TWO;
			user->ntoread = 2;
			bufferevent_setwatermark(bev, EV_READ, user->ntoread, user->ntoread);
		}
		if (payload_len == 127) {
			user->step = TWO;
			user->ntoread = 8;
			bufferevent_setwatermark(bev, EV_READ, user->ntoread, user->ntoread);
		}
		break;

	case TWO:
		payload_len = user->message_frame_header.payload_len;
		if (payload_len == 126) {
			payload_len = ntohs(*(uint16_t*)tmp);
		}
		if (payload_len == 127) {
			payload_len = myntohll(*(uint64_t*)tmp);
		}
		if (payload_len > MAX_MSG_LEN || user->content.size() + payload_len > MAX_MSG_LEN) {
			close_conn(user);
			return;
		}
		user->step = THREE;
		user->ntoread = 4 + payload_len;
		bufferevent_setwatermark(bev, EV_READ, user->ntoread, user->ntoread);
		break;

	case THREE:
		payload_len = user->message_frame_header.payload_len;
		if (payload_len > 0) {
			if (unmask_payload_data(tmp, tmp + 4, payload_len) == 0) {
				user->content += string(tmp + 4, payload_len);
			}
		}

		if (user->message_frame_header.opcode = 0x8) {
			close_conn(user);
			return;
		}

		//if it is the last frame of message, send the content
		if (user->message_frame_header.fin == 1) {
			if (!user->content.empty()) {
				char *message_frame_buf = NULL;
				uint32_t buflen = 0;
				generate_message_frame(user->content.c_str(), user->content.size(), &message_frame_buf, &buflen);
				if (message_frame_buf && buflen > 0) {
					//send a message to each user
					for (int32_t i = 0; i < user_vec.size(); ++i) {
						if (bufferevent_write(user_vec[i]->bev, message_frame_buf, buflen) == 0) {
							LOG("i send a message");
						}
					}
				}
				user->content = "";
			}
		}

		user->step = ONE;
		user->ntoread = 2;
		bufferevent_setwatermark(bev, EV_READ, user->ntoread, user->ntoread);
		break;

	default:
		exit(-1);
		break;
	}
}


void user_disconnect_cb(struct bufferevent *bev, short what, void *ctx) {
	user_t *user = (user_t*)ctx;
	close_conn(user);
}


void read_websocket_request(struct bufferevent *bev, void *ctx) {
	//LOG("%s", __func__);
	user_t *user = (user_t*)ctx;
	char tmp[WS_REQ_ONCE_READ + 1] = {0};
	bufferevent_read(bev, tmp, WS_REQ_ONCE_READ);

	user->ws_req_str += tmp;

	//check if receive completely
	int32_t n = user->ws_req_str.size();
	if (n > MAX_WS_REQ_LEN) {
		close_conn(user);
	}

	//check if recv done
	if (n >= 4 && user->ws_req_str[n - 4] == '\r' && user->ws_req_str[n - 3] == '\n' && user->ws_req_str[n - 2] == '\r' && user->ws_req_str[n - 1] == '\n') {
		//parse and responsd
		ws_req_t ws_req;
		parse_websocket_request(user->ws_req_str.c_str(), &ws_req);
		string resp = generate_websocket_response(&ws_req);
		bufferevent_write(bev, resp.c_str(), resp.length());

		//start reading message
		user->step = ZERO;
		user->ntoread = 2;
		bufferevent_setcb(bev, read_message_data, NULL, NULL, user);
		bufferevent_setwatermark(bev, EV_READ, user->ntoread, user->ntoread);

		cout << user->ws_req_str;
		cout << resp;
	}
}


void listencb(struct evconnlistener *listener, evutil_socket_t clisockfd, struct sockaddr *addr, int len, void *ptr) {
	struct event_base *eb = evconnlistener_get_base(listener);
	assert(eb);
	struct bufferevent *bev = bufferevent_socket_new(eb, clisockfd, BEV_OPT_CLOSE_ON_FREE);
	assert(bev);

	//create a user
	user_t *user = new user_t;
	user->bev = bev;
	user->step = ZERO;
	user->ntoread = WS_REQ_ONCE_READ;
	//bev_user_map[bev] = user;
	user_vec.push_back(user);

	bufferevent_setcb(user->bev, read_websocket_request, NULL, user_disconnect_cb, user);
	bufferevent_setwatermark(user->bev, EV_READ, WS_REQ_ONCE_READ, WS_REQ_ONCE_READ);
	bufferevent_enable(bev, EV_READ | EV_WRITE);
}


int main() {
	//initialize
	setbuf(stdout, NULL);
	base = event_base_new();
	assert(base);

	struct sockaddr_in srvaddr;
	srvaddr.sin_family = AF_INET;
	srvaddr.sin_addr.s_addr = INADDR_ANY;
	srvaddr.sin_port = htons(10086);

	listener = evconnlistener_new_bind(base, listencb, NULL, LEV_OPT_REUSEABLE | LEV_OPT_CLOSE_ON_FREE, 500, (const struct sockaddr*)&srvaddr, sizeof(struct sockaddr));
	assert(listener);

	event_base_dispatch(base);

	return 0;
}
