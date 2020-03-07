
/*
 * main.cpp
 *
 *  Created on: Oct 1, 2014
 *      Author: awahl
 */

#include <unistd.h>
#include <iostream>
#include <spdlog/sinks/stdout_color_sinks.h>
#include "queue.h"
#include "poll.h"
#include "timer.h"
#include "httpserver.h"


namespace spd = spdlog;

int usage(char *argv[])
{
    std::cerr << "usage: " << argv[0] << " -b [broker]" << std::endl;
    return -1;
}

int main(int argc, char *argv[])
{
	int opt;
    auto logger = spd::stdout_color_mt("logger");

	char *dest = nullptr;
    const char *filter = "not port 22 and not port 9092";
    const char *dev = "eth0";
    const char *topic = "packets";
    char *broker = nullptr;

	while((opt = getopt(argc, argv, "i:f:t:b:")) != -1) {
		switch(opt) {
		case 'i':
            logger->info("dev: {}", optarg);
            dev = optarg;
			break;
		case 'f':
            logger->info("filter: {}", optarg);
			filter = optarg;
			break;
        case 't':
            logger->info("topic: {}", optarg);
            topic = optarg;
            break;
        case 'b':
            if (optarg == nullptr)
                return usage(argv);
            logger->info("broker: {}", optarg);
            broker = optarg;
            break;
		default:
			std::cerr << "error" << std::endl;
			break;
		}
    	}
    logger->set_level(spd::level::info);
	BasicSystem s(1000);
	EpollTask epollTask(&s, "epollTask", 100, 100);
	TimerTask timerTask(&s);
	HttpServer httpServer(&s);

	timerTask.config.set("timeout", "500");
	s.getScheduler()->setIdleTask(&epollTask);
	Thread thread1(s.getScheduler());

	Message *msg = s.getMessage();
	msg->type = initMsg;
	msg->dst = &httpServer;
	s.postMsg(msg);

	thread1.loop();

	return 0;
}
