/**
 *
 * filename: demo.h
 * summary:
 * author: caosiyang
 * email: csy3228@gmail.com
 *
 */
#ifndef DEMO_H
#define DEMO_H

#include "../websocket.h"
#include "../connection.h"
#include "user.h"
#include "event2/event.h"
#include "event2/listener.h"
#include "event2/bufferevent.h"
#ifdef WIN32
#include <windows.h>
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#endif
#include <assert.h>
#include <signal.h>
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <map>
using namespace std;



#endif
