#include "user.h"


extern vector<user_t*> user_vec;


user_t *user_create() {
	user_t *user = new (nothrow) user_t;
	if (user) {
		user->id = 0;
		user->wscon = ws_conn_new();
		user->msg = "";
	}
	return user;
}


void user_destroy(user_t *user) {
	if (user) {
		if (user->wscon) {
			ws_conn_free(user->wscon);
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

		frame_buffer_t *fb = frame_buffer_new(1, 1, user->wscon->frame->payload_len, user->wscon->frame->payload_data);

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
