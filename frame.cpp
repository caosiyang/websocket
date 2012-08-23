#include "frame.h"


frame_t *frame_new() {
	frame_t *frame = new (nothrow) frame_t;
	if (frame) {
		memset(frame, 0, sizeof(frame_t));
	}
	return frame;
}


void frame_free(frame_t *frame) {
	if (frame) {
		if (frame->payload_data) {
			delete[] frame->payload_data;
		}
		delete frame;
	}
}


bool is_frame_valid(const frame_t *frame) {
	if (frame && frame->fin <= 1 && frame->opcode <= 0xf && frame->mask == 1) {
		return true;
	}
	return false;
}


#if 0
int32_t frame_set(frame_t *frame, uint8_t fin, uint8_t opcode, uint64_t payload_len, const char *payload_data) {
	if (!frame || fin > 1 || opcode > 0xf) {
		return -1;
	}

	frame->fin = fin;
	frame->opcode = opcode;
	frame->mask = 0;
	if (payload_data && payload_len > 0) {
		frame->payload_len = payload_len;
		frame->payload_data = new char[payload_len];
		memcpy(frame->payload_data, payload_data, payload_len);
	} else {
		frame->payload_len = 0;
		frame->payload_data = NULL;
	}
	return 0;
}
#endif


frame_buffer_t *frame_buffer_new(uint8_t fin, uint8_t opcode, uint64_t payload_len, const char *payload_data) {
	if (fin > 1 || opcode > 0xf) {
		return NULL;
	}

	uint8_t mask = 0; //must not mask at server endpoint
	char masking_key[4] = {0}; //no need at server endpoint

	char *p = NULL; //buffer
	uint64_t len = 0; //buffer length

	unsigned char c1 = 0x00;
	unsigned char c2 = 0x00;
	c1 = c1 | (fin << 7); //set fin
	c1 = c1 | opcode; //set opcode
	c2 = c2 | (mask << 7); //set mask

	if (!payload_data || payload_len == 0) {
		if (mask == 0) {
			p = new char[2];
			*p = c1;
			*(p + 1) = c2;
			len = 2;
		} else {
			p = new char[2 + 4];
			*p = c1;
			*(p + 1) = c2;
			memcpy(p + 2, masking_key, 4);
			len = 2 + 4;
		}
	} else if (payload_data && payload_len <= 125) {
		if (mask == 0) {
			p = new char[2 + payload_len];
			*p = c1;
			*(p + 1) = c2 + payload_len;
			memcpy(p + 2, payload_data, payload_len);
			len = 2 + payload_len;
		} else {
			p = new char[2 + 4 + payload_len];
			*p = c1;
			*(p + 1) = c2 + payload_len;
			memcpy(p + 2, masking_key, 4);
			memcpy(p + 6, payload_data, payload_len);
			len = 2 + 4 + payload_len;
		}
	} else if (payload_data && payload_len >= 126 && payload_len <= 65535) {
		if (mask == 0) {
			p = new char[4 + payload_len];
			*p = c1;
			*(p + 1) = c2 + 126;
			uint16_t tmplen = myhtons((uint16_t)payload_len);
			memcpy(p + 2, &tmplen, 2);
			memcpy(p + 4, payload_data, payload_len);
			len = 4 + payload_len;
		} else {
			p = new char[4 + 4 + payload_len];
			*p = c1;
			*(p + 1) = c2 + 126;
			uint16_t tmplen = myhtons((uint16_t)payload_len);
			memcpy(p + 2, &tmplen, 2);
			memcpy(p + 4, masking_key, 4);
			memcpy(p + 8, payload_data, payload_len);
			len = 4 + 4 + payload_len;
		}
	} else if (payload_data && payload_len >= 65536) {
		if (mask == 0) {
			p = new char[10 + payload_len];
			*p = c1;
			*(p + 1) = c2 + 127;
			uint64_t tmplen = myhtonll(payload_len);
			memcpy(p + 2, &tmplen, 8);
			memcpy(p + 10, payload_data, payload_len);
			len = 10 + payload_len;
		} else {
			p = new char[10 + 4 + payload_len];
			*p = c1;
			*(p + 1) = c2 + 127;
			uint64_t tmplen = myhtonll(payload_len);
			memcpy(p + 2, &tmplen, 8);
			memcpy(p + 10, masking_key, 4);
			memcpy(p + 14, payload_data, payload_len);
			len = 10 + 4 + payload_len;
		}
	}

	frame_buffer_t *fb = NULL;
	if (p && len > 0) {
		fb = new (nothrow) frame_buffer_t;
		if (fb) {
			fb->data = p;
			fb->len = len;
		}
	}
	return fb;
}


frame_buffer_t *frame_buffer_new(const frame_t *frame) {
	if (!frame || frame->fin > 1 || frame->opcode > 0xf || frame->mask > 1) {
		return NULL;
	}
	frame_buffer_t *fb = frame_buffer_new(frame->fin, frame->opcode, frame->payload_len, frame->payload_data);
	return fb;
}


void frame_buffer_free(frame_buffer_t *fb) {
	if (fb) {
		if (fb->data) {
			delete[] fb->data;
		}
		delete fb;
	}
}


void print_frame_info(const frame_buffer_t *fb) {
	if (!fb || !fb->data) {
		return;
	}
	LOG("--------------------");
	char *p = fb->data;
	uint8_t fin = 0, opcode = 0, mask = 0;
	uint64_t payload_len = 0;
	fin = (*(uint8_t*)p) >> 7;
	opcode = (*(uint8_t*)p) & 0x0f;
	mask = (*(uint8_t*)(p + 1)) >> 7;
	payload_len = (*(uint8_t*)(p + 1)) & 0x7f;
	char *tmp = NULL;
	if (payload_len == 0) {
	} else if (payload_len <= 125) {
		tmp = new char[payload_len + 1];
		tmp[payload_len] = 0;
		memcpy(tmp, p + 2, payload_len);
	} else if (payload_len == 126) {
		LOG("126");
		payload_len = myntohs(*(uint16_t*)(p + 2));
		tmp = new char[payload_len + 1];
		tmp[payload_len] = 0;
		memcpy(tmp, p + 2 + 2, payload_len);
	} else if (payload_len == 127) {
		LOG("127");
		payload_len = myntohll(*(uint64_t*)(p + 2));
		tmp = new char[payload_len + 1];
		tmp[payload_len] = 0;
		memcpy(tmp, p + 2 + 8, payload_len);
	}
	LOG("fin = %lu", fin);
	LOG("opcode = %lu", opcode);
	LOG("mask = %lu", mask);
	LOG("payload_len = %lu", payload_len);
	if (tmp) {
		LOG("payload = \n%s", tmp);
		delete[] tmp;
	} else {
		LOG("payload = NULL");
	}
	LOG("--------------------");
}
