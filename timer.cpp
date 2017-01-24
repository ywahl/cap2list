/*
 * timer.cpp
 *
 *  Created on: Oct 26, 2014
 *      Author: awahl
 */

#include <iostream>
#include "timer.h"

void TimerTask::execute(Message *msg)
{
	switch(msg->type) {
	case initMsg:
		{
			Task *eventTask = static_cast<Task *>(msg->data);
			std::cout << "Timer creation" << std::endl;
			init(eventTask);
		}
		break;
	case readMsg:
		{
			EpollMsgData *data = reinterpret_cast<EpollMsgData *>(msg->smallData);
			int len;
			char *buffer;
			while(1) {
				if (!(buffer = EpollTask::readBuffer(this, msg->src, data->fd, &len)))
					break;

				u_int64_t timeouts = *reinterpret_cast<u_int64_t *>(buffer);

				if (timeouts > 1) {
					//throw std::runtime_error("more than 1 timeout occured between timer reads");
					std::cerr << "more than 1 timeout occured between timer reads: " << timeouts << std::endl;
				}

				bufferPool.releaseBuffer(buffer);
			}
			notifyTasks();
		}
		break;
	case subscribeMsg:
		{
			Task *subscribedTask = static_cast<Task *>(msg->data);
			TimerData *tmrData = pool.getBuffer();
			tmrData->task = subscribedTask;
			tmrData->timeout = *reinterpret_cast<int *>(msg->smallData);
			tmrData->type = *reinterpret_cast<TimerType *>(msg->smallData + sizeof(tmrData->timeout));
			notifyTaskList.push_back(tmrData);
			std::cout << "Timer task: registering " << subscribedTask->getId() << std::endl;
		}
		break;
	default:
		throw std::runtime_error("Unexpected message");
	}
	system->releaseMessage(msg);
}

void TimerTask::init(Task *eventTask)
{
	clock_fd = timerfd_create(CLOCK_MONOTONIC, 0);
	make_socket_non_blocking(clock_fd);
	struct itimerspec tm_value;

	memset(&tm_value, 0, sizeof(tm_value));
	//default timeout 1ms
	long timeOutMs = (long)config.get("timeout", 1);
	tm_value.it_value.tv_nsec = timeOutMs * 1000000;
	tm_value.it_interval.tv_nsec = timeOutMs * 1000000;

	timerfd_settime(clock_fd, 0, &tm_value, NULL);

	std::cout << "timer taskclock_fd=" << clock_fd << " timeoutms=" << timeOutMs << std::endl;
	Message *msg = EpollTask::prepareMsg(this, eventTask, subscribeMsg, clock_fd ,EPOLLIN);
	EpollMsgData *data = reinterpret_cast<EpollMsgData *>(msg->smallData);
	data->state = fdstate_connected;

	system->postMsg(msg);

}

void TimerTask::notifyTasks()
{
	ticks++;
	for (auto *tmrData : notifyTaskList) {
		if ((ticks % tmrData->timeout) == 0) {
			Message *msg = getMessage(tmrData->task, timeOutMsg);
			system->postMsg(msg);
		}
	}
}
