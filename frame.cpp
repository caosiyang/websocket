#include "frame.h"


frame_t *frame_new() {
	frame_t *frame = new (nothrow) frame_t;
	if (frame) {
		memset(frame, 0, sizeof(frame_t));
	}
	return frame;
}


void frame_bzero(frame_t *frame) {
	if (frame) {
		memset(frame, 0, sizeof(frame_t));
	}
}


void frame_free(frame_t *frame) {
	if (frame) {
		if (frame->payload_data) {
			delete[] frame->payload_data;
		}
		delete frame;
	}
}


frame_buffer_t *frame_buffer_new() {
	frame_buffer_t *fb = new (nothrow) frame_buffer_t;
	if (fb) {
		fb->data = NULL;
		fb->len = 0;
	}
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

