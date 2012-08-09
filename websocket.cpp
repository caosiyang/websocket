#include "websocket.h"


int32_t parse_websocket_request(const char *s_req, ws_req_t *ws_req) {
	if (!s_req || !ws_req) {
		return -1;
	}
	int len = strlen(s_req);
	char tmp[len + 1];
	tmp[len] = 0;
	memcpy(tmp, s_req, len);

	char *delim = "\r\n";
	char *p = NULL, *q = NULL;

	p = strtok(tmp, delim);
	if (p) {
		//printf("%s\n", p);
		ws_req->req = p;
		while (p = strtok(NULL, delim)) {
			//printf("%s\n", p);
			if ((q = strstr(p, ":")) != NULL) {
				*q = '\0';
				if (strcasecmp(p, "Connection") == 0) {
					while (*++q == ' ');
					ws_req->connection = q;
				}
				if (strcasecmp(p, "Upgrade") == 0) {
					while (*++q == ' ');
					ws_req->upgrade = q;
				}
				if (strcasecmp(p, "Host") == 0) {
					while (*++q == ' ');
					ws_req->host = q + 1;
				}
				if (strcasecmp(p, "Origin") == 0) {
					while (*++q == ' ');
					ws_req->origin = q;
				}
				if (strcasecmp(p, "Cookie") == 0) {
					while (*++q == ' ');
					ws_req->cookie = q;
				}
				if (strcasecmp(p, "Sec-WebSocket-Key") == 0) {
					while (*++q == ' ');
					ws_req->sec_websocket_key = q;
				}
				if (strcasecmp(p, "Sec-WebSocket-Version") == 0) {
					while (*++q == ' ');
					ws_req->sec_websocket_version = q;
				}
			}
		}
	}
	return 0;
}


void print_websocket_request(const ws_req_t *ws_req) {
	if (ws_req) {
		if (!ws_req->req.empty()) {
			fprintf(stdout, "%s\r\n", ws_req->req.c_str());
		}
		if (!ws_req->connection.empty()) {
			fprintf(stdout, "Connection: %s\r\n", ws_req->connection.c_str());
		}
		if (!ws_req->upgrade.empty()) {
			fprintf(stdout, "Upgrade: %s\r\n", ws_req->upgrade.c_str());
		}
		if (!ws_req->host.empty()) {
			fprintf(stdout, "Host: %s\r\n", ws_req->host.c_str());
		}
		if (!ws_req->origin.empty()) {
			fprintf(stdout, "Origin: %s\r\n", ws_req->origin.c_str());
		}
		if (!ws_req->cookie.empty()) {
			fprintf(stdout, "Cookie: %s\r\n", ws_req->cookie.c_str());
		}
		if (!ws_req->sec_websocket_key.empty()) {
			fprintf(stdout, "Sec-WebSocket-Key: %s\r\n", ws_req->sec_websocket_key.c_str());
		}
		if (!ws_req->sec_websocket_version.empty()) {
			fprintf(stdout, "Sec-WebSocket-Version: %s\r\n", ws_req->sec_websocket_version.c_str());
		}
		fprintf(stdout, "\r\n");
	}
}


string generate_websocket_response(const ws_req_t *ws_req) {
	string resp;
	resp += "HTTP/1.1 101 WebSocket Protocol HandShake\r\n";
	resp += "Connection: Upgrade\r\n";
	resp += "Upgrade: WebSocket\r\n";
	resp += "Server: WebChat Demo Server\r\n";
	resp += "Sec-WebSocket-Accept: " + generate_key(ws_req->sec_websocket_key) + "\r\n";
	resp += "\r\n";
	return resp;
}


string generate_key(const string &key) {
	//sha-1
	string tmp = key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
	unsigned char md[20] = {0};
	SHA1((const unsigned char*)tmp.c_str(), tmp.length(), md);

	//base64 encode
	string res = base64_encode(md, 20);

	return res;
}


int32_t parse_frame_header(const char *buf, frame_t *frame) {
	if (!buf || !frame) {
		return -1;
	}
	unsigned char c1 = *buf;
	unsigned char c2 = *(buf + 1);
	frame->fin = (c1 >> 7) & 0xff;
	frame->opcode = c1 & 0x0f;
	frame->mask = (c2 >> 7) & 0xff;
	frame->payload_len = c2 & 0x7f;
	return 0;
}


int32_t unmask_payload_data(frame_t *frame) {
	if (frame && frame->payload_data && frame->payload_len > 0) {
		for (int32_t i = 0; i < frame->payload_len; ++i) {
			*(frame->payload_data + i) = *(frame->payload_data + i) ^ *(frame->masking_key + i % 4);
		}
		return 0;
	}
	return -1;
}


int32_t unmask_payload_data(const char *masking_key, char *payload_data, uint32_t payload_len) {
	if (!masking_key || !payload_data || payload_len == 0) {
		return -1;
	}
	for (int32_t i = 0; i < payload_len; ++i) {
		*(payload_data + i) = *(payload_data + i) ^ *(masking_key + i % 4);
	}
	return 0;
}


frame_buffer_t *generate_frame_buffer(const frame_t *frame) {
	if (!frame) {
		return NULL;
	}

	if (frame->fin > 1) {
		return NULL;
	}

	if (frame->opcode > 0xf) {
		return NULL;
	}

	if (frame->mask > 1) {
		return NULL;
	}

	if (frame->payload_data != NULL && frame->payload_len == 0) {
		return NULL;
	}

	if (frame->payload_data == NULL && frame->payload_len != 0) {
		return NULL;
	}

	frame_buffer_t *fb = NULL;
	char *p = NULL;
	unsigned char c1 = 0x00;
	unsigned char c2 = 0x00; //mask = 0
	c1 = c1 | (frame->fin << 7); //set fin
	c1 = c1 | frame->opcode; //set opcode
	c2 = c2 | (frame->mask << 7); //set mask

	if (frame->payload_len == 0) {
		if (frame->mask == 0) {
			p = new char[2];
			*p = c1;
			*(p + 1) = c2;
			fb = frame_buffer_new();
			fb->data = p;
			fb->len = 2;
		} else {
			p = new char[2 + 4];
			*p = c1;
			*(p + 1) = c2;
			memcpy(p + 2, frame->masking_key, 4);
			fb = frame_buffer_new();
			fb->data = p;
			fb->len = 2 + 4;
		}

	} else if (frame->payload_data && frame->payload_len <= 125) {
		if (frame->mask == 0) {
			p = new char[2 + frame->payload_len];
			*p = c1;
			*(p + 1) = c2 + frame->payload_len;
			memcpy(p + 2, frame->payload_data, frame->payload_len);
			fb = frame_buffer_new();
			fb->data = p;
			fb->len = 2 + frame->payload_len;
		} else {
			p = new char[2 + 4 + frame->payload_len];
			*p = c1;
			*(p + 1) = c2 + frame->payload_len;
			memcpy(p + 2, frame->masking_key, 4);
			memcpy(p + 6, frame->payload_data, frame->payload_len);
			fb = frame_buffer_new();
			fb->data = p;
			fb->len = 2 + 4 + frame->payload_len;
		}

	} else if (frame->payload_data && frame->payload_len >= 126 && frame->payload_len <= 65535) {
		if (frame->mask == 0) {
			p = new char[4 + frame->payload_len];
			*p = c1;
			*(p + 1) = c2 + 126;
			uint16_t tmplen = myhtons((uint16_t)frame->payload_len);
			memcpy(p + 2, &tmplen, 2);
			memcpy(p + 4, frame->payload_data, frame->payload_len);
			fb = frame_buffer_new();
			fb->data = p;
			fb->len = 4 + frame->payload_len;
		} else {
			p = new char[4 + 4 + frame->payload_len];
			*p = c1;
			*(p + 1) = c2 + 126;
			uint16_t tmplen = myhtons((uint16_t)frame->payload_len);
			memcpy(p + 2, &tmplen, 2);
			memcpy(p + 4, frame->masking_key, 4);
			memcpy(p + 8, frame->payload_data, frame->payload_len);
			fb = frame_buffer_new();
			fb->data = p;
			fb->len = 4 + 4 + frame->payload_len;
		}

	} else if (frame->payload_data && frame->payload_len >= 65536) {
		if (frame->mask == 0) {
			p = new char[10 + frame->payload_len];
			*p = c1;
			*(p + 1) = c2 + 127;
			uint64_t tmplen = myhtonll(frame->payload_len);
			memcpy(p + 2, &tmplen, 8);
			memcpy(p + 10, frame->payload_data, frame->payload_len);
			fb = frame_buffer_new();
			fb->data = p;
			fb->len = 10 + frame->payload_len;
		} else {
			p = new char[10 + 4 + frame->payload_len];
			*p = c1;
			*(p + 1) = c2 + 127;
			uint64_t tmplen = myhtonll(frame->payload_len);
			memcpy(p + 2, &tmplen, 8);
			memcpy(p + 10, frame->masking_key, 4);
			memcpy(p + 14, frame->payload_data, frame->payload_len);
			fb = frame_buffer_new();
			fb->data = p;
			fb->len = 10 + 4 + frame->payload_len;
		}

	}

	return fb;
}
