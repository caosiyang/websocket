/**
 *
 * filename: user.h
 * summary:
 * author: caosiyang
 * email: csy3228@gmail.com
 *
 */
#ifndef USER_H
#define USER_H

#include "../connection.h"


typedef struct user {
	uint32_t id;
	ws_conn_t *wscon;
	string msg;
} user_t;


user_t *user_create();


void user_destroy(user_t *user);


void frame_recv_cb(void *arg);


#endif
