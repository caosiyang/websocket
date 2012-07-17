#include "websocket.h"


int32_t parse_websocket_request(const char *s_req, ws_req_t *ws_req) {
	if (!s_req || !ws_req) {
		return -1;
	}
	int len = strlen(s_req) + 1;
	char tmp[len];
	memcpy(tmp, s_req, len);

	char *dilim = "\r\n";
	char *p = NULL, *q = NULL;

	p = strtok(tmp, dilim);
	if (p) {
		ws_req->req = p;
		while (p = strtok(NULL, dilim)) {
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


static string generate_key(const string &key) {
	//sha-1
	string tmp = key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
	unsigned char md[20] = {0};
	SHA1((const unsigned char*)tmp.c_str(), tmp.length(), md);

	//base64 encode
	string res = base64_encode(md, 20);

	return res;
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


int32_t read_fin_opcode_mask_payloadlen(const char *buf, message_frame_header_t *header) {
	if (!buf || !header) {
		return -1;
	}
	unsigned char c1 = *buf;
	unsigned char c2 = *(buf + 1);
	header->fin = (c1 >> 7) & 0xff;
	header->opcode = c1 & 0x0f;
	header->mask = (c2 >> 7) & 0xff;
	header->payload_len = c2 & 0x7f;
	return 0;
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


int32_t generate_message_frame(const char *payload_data, uint32_t payload_len, char **message_frame, uint32_t *message_frame_len) {
	if (!payload_data) {
		payload_len = 0;
	}
	if (payload_len == 0) {
		*message_frame = NULL;
		*message_frame_len = 0;
		return 0;
	}

	char *p = NULL;
	unsigned char c1 = 0x81; //fin = 1, opcode = 1
	unsigned char c2 = 0x00; //mask = 0

	if (payload_len <= 125) {
		p = new char[2 + payload_len];
		c2 = payload_len;
		//fill
		*p = c1;
		*(p + 1) = c2;
		memcpy(p + 2, payload_data, payload_len);
		*message_frame = p;
		*message_frame_len = 2 + payload_len;
	} else if (payload_len >= 126 && payload_len <= 65535) {
		p = new char[4 + payload_len];
		c2 = 126;
		//fill
		*p = c1;
		*(p + 1) = c2;
		uint16_t tmplen = payload_len;
		tmplen = myhtons(tmplen);
		memcpy(p + 2, &tmplen, 2);
		memcpy(p + 4, payload_data, payload_len);
		*message_frame = p;
		*message_frame_len = 4 + payload_len;
	} else if (payload_len >= 65536) {
		p = new char[10 + payload_len];
		c2 = 127;
		//fill
		*p = c1;
		*(p + 1) = c2;
		uint64_t tmplen = payload_len;
		tmplen = myhtonll(tmplen);
		memcpy(p + 2, &tmplen, 8);
		memcpy(p + 10, payload_data, payload_len);
		*message_frame  = p;
		*message_frame_len = 10 + payload_len;
	}
	return 0;
}
