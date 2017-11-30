
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
#include "timer.h"
#include "pcapintf.h"
#include <spdlog/spdlog.h>


namespace spd = spdlog;

int main(int argc, char *argv[])
{
	int opt;
    auto logger = spd::stdout_color_mt("logger");
    spd::register_logger(logger);
	char *dest = nullptr;

	while((opt = getopt(argc, argv, "sc:")) != -1) {
		switch(opt) {
		case 's':
			break;
		case 'c':
            logger->info("args optarg: {}", optarg);
			dest = optarg;
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

	return 0;
}
