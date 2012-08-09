#include "user.h"


extern vector<user_t*> user_vec;


user_t *user_create() {
	user_t *user = new (nothrow) user_t;
	if (user) {
		user->id = 0;
		user->wscon = ws_conn_create();
		user->msg = "";
	}
	return user;
}


void user_destroy(user_t *user) {
	if (user) {
		if (user->wscon) {
			ws_conn_destroy(user->wscon);
		}
		delete user;
	}
}


void frame_recv_cb(void *arg) {
	user_t *user = (user_t*)arg;
	if (user->wscon->frame->payload_len > 0) {
		user->msg += string(user->wscon->frame->payload_data, user->wscon->frame->payload_len);
	}
	if (user->wscon->frame->fin == 1) {
		LOG("%s", user->msg.c_str());

		user->wscon->frame->fin = 1;
		user->wscon->frame->opcode = 1;
		user->wscon->frame->mask = 0;

		//printf("fin %lu\n", user->wscon->frame->fin);
		//printf("opcode %lu\n", user->wscon->frame->opcode);
		//printf("mask %lu\n", user->wscon->frame->mask);
		//printf("payload_len %lu\n", user->wscon->frame->payload_len);

		frame_buffer_t *fb = generate_frame_buffer(user->wscon->frame);

		if (fb) {
			//send to other users
			for (int32_t i = 0; i < user_vec.size(); ++i) {
				if (user_vec[i] != user) {
#if 1
					if (send_a_frame(user_vec[i]->wscon, fb) == 0) {
						LOG("i send a message");
					}
#endif
				}
			}

			frame_buffer_free(fb);
		}

		user->msg = "";
	}
}

