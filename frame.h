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

#include <string.h>
#include <iostream>
#include "tools.h"
using namespace std;


//frame buffer
typedef struct FrameBuffer {
	char *data;
	uint64_t len;
} frame_buffer_t;


//frame
typedef struct Frame {
	uint8_t fin;
	uint8_t opcode;
	uint8_t mask;
	uint64_t payload_len;
	unsigned char masking_key[4];
	char *payload_data;
} frame_t;


frame_t *frame_new();


void frame_free(frame_t *frame);


bool is_frame_valid(const frame_t *frame);


#if 0
int32_t frame_set(frame_t *frame,
		uint8_t fin,
		uint8_t opcode,
		uint64_t payload_len,
		const char *payload_data);
#endif


frame_buffer_t *frame_buffer_new(uint8_t fin,
		uint8_t opcode,
		uint64_t payload_len,
		const char *payload_data);


frame_buffer_t *frame_buffer_new(const frame_t *frame);


void frame_buffer_free(frame_buffer_t *fb);


void print_frame_info(const frame_buffer_t *fb);


#endif
