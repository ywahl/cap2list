/*
 * poll.h
 *
 *  Created on: Sep 17, 2014
 *      Author: awahl
 */

#ifndef POLL_H_
#define POLL_H_
#include <sys/epoll.h>
#include <memory.h>
#include <errno.h>
#include <unistd.h>
#include "system.h"

#define MAX_FD_SIZE 200
#define NUM_BUFFERS 10000

enum {
	fdstate_init,
	fdstate_connected
};



struct EpollMsgData {
	int fd;
	int info;
	int state;
	bool autocreate;
};


struct EpollFD {
	int fd;
	Task *task;
	int ops;
	int state;
	bool autocreate;
};

extern BufferPool<char> bufferPool;

class EpollTask : public Task {
private:
	int efd;
	int maxEvents;
	int maxPoll;

	struct epoll_event *events;
	//Dangerous!!!!
	EpollFD fds[MAX_FD_SIZE];
	//buffer pool;
public:
	EpollTask(System *s, const char *n, int maxE, int maxpoll);
	void execute(Message *d);
	void subscribe(const EpollMsgData *, Task *src);
	void unSubscribe(const EpollMsgData *, Task *src);
	int acceptNewConnection(EpollFD *fd);

	void poll(int w);

	//public static helper functions
	static char *readBuffer(Task *srcTask, Task *epollTask, int fd, int *len);
	static Message *prepareMsg(Task *srcTsk, Task *dstTsk, MsgType msgType, int fd, int info, bool acreate = false) {

		Message *msg = srcTsk->getMessage(dstTsk, msgType);

		EpollMsgData *msgData = reinterpret_cast<EpollMsgData *>(msg->smallData);
		msgData->fd = fd;
		msgData->info = info;
		msgData->state = fdstate_init;
		msgData->autocreate = acreate;

		return msg;
	}
};

#define MAX_BUFFER_SZ 2048


int make_socket_non_blocking (int sfd);

#endif /* POLL_H_ */
