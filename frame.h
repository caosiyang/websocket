/**
 *
 * filename: frame.h
 * summary:
 * author: caosiyang
 * email:  csy3228@gmail.com
 *
 */
#ifndef FRAME_H
#define FRAME_H

#include <iostream>
using namespace std;


//frame
typedef struct Frame {
	uint8_t fin;
	uint8_t opcode;
	uint8_t mask;
	uint64_t payload_len;
	unsigned char masking_key[4];
	char *payload_data;
} frame_t;


//frame buffer
typedef struct FrameBuffer {
	char *data;
	uint64_t len;
} frame_buffer_t;


frame_t *frame_new();


void frame_bzero(frame_t *frame);


void frame_free(frame_t *frame);


frame_buffer_t *frame_buffer_new();


void frame_buffer_free(frame_buffer_t *fb);


#endif
