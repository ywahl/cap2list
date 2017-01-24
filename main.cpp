
/*
 * main.cpp
 *
 *  Created on: Oct 1, 2014
 *      Author: awahl
 */
#include <unistd.h>
#include <iostream>
#include "queue.h"
#include "poll.h"
#include "sockets.h"
#include "timer.h"
#include "config.h"
#include "test.h"
#include "pcapintf.h"


int main(int argc, char *argv[])
{
	int opt;
	bool server = true;
	char *dest = NULL;
	std::cout << sizeof(int) << " " << sizeof(long) << std::endl;

	while((opt = getopt(argc, argv, "sc:")) != -1) {
		switch(opt) {
		case 's':
			server = true;
			break;
		case 'c':
			dest = optarg;
			server= false;
			break;
		default:
			std::cerr << "error" << std::endl;
			break;
		}
	}

	BasicSystem s(1000);
	EpollTask epollTask(&s, "epollTask", 100, 100);
	TimerTask timerTask(&s);
	timerTask.config.set("timeout", "500");
	s.getScheduler()->setIdleTask(&epollTask);
	Thread thread1(s.getScheduler());

	Message *msg = s.getMessage();
	msg->type = initMsg;
	msg->dst = &timerTask;
	msg->data = &epollTask;
	s.postMsg(msg);
	PcapInterface pcapTask(&s);
	pcapTask.config.set("dev", "wlp2s0");
	msg = s.getMessage();
	msg->dst = &pcapTask;
	msg->type = initMsg;
	s.postMsg(msg);
	thread1.loop();


//
//	if (server) {
//	  TestServer testTask(&s);
//	  std::cout << "Running in server mode" << std::endl;
//	  testTask.config.set("server", "true");
//	  testTask.config.set("ipaddress", "127.0.0.1");
//	  bool testValue = testTask.config.get("server");
//	  msg = s.getMessage();
//	  msg->dst = &testTask;
//	  msg->type = initMsg;
//	  s.postMsg(msg);
//	  thread1.loop();
//	} else {
//	  TestClient testTask(&s);
//	  std::cout << "Running in client mode" << std::endl;
//	  testTask.config.set("ipaddress", "127.0.0.1");
//	  msg = s.getMessage();
//	  msg->dst = &testTask;
//	  msg->type = initMsg;
//	  s.postMsg(msg);
//	  thread1.loop();
//	}

	return 0;
}
