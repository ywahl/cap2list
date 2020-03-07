/*
 * poll.cpp
 *
 *  Created on: Sep 15, 2014
 *      Author: awahl
 */
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <algorithm>

#include "queue.h"
#include "poll.h"

BufferPool<char> bufferPool(NUM_BUFFERS, MAX_BUFFER_SZ, "BUFP");


int make_socket_non_blocking (int sfd)
{
  int flags, s;

  flags = fcntl (sfd, F_GETFL, 0);
  if (flags == -1)
  {
      return -1;
  }

  flags |= O_NONBLOCK;
  s = fcntl (sfd, F_SETFL, flags);
  if (s == -1)
  {
  }
  return 0;
}


EpollTask::EpollTask(System *s, const char *n, int maxE, int maxpoll) : Task(s, n), maxEvents(maxE), maxPoll(maxpoll)
{
	efd = epoll_create(1);
	events = new epoll_event[maxEvents];
	memset(&fds, 0, sizeof(struct EpollFD) * MAX_FD_SIZE);
}



void EpollTask::execute(Message *msg)
{
	int w = 0;
	if (!msg) {
		//idle
		poll(w);
		return;
	}
	EpollMsgData *data = reinterpret_cast<EpollMsgData *>(msg->smallData);

	switch(msg->type) {
	case subscribeMsg:
		subscribe(data, msg->src);
		break;

	case unSubscribeMsg:
		unSubscribe(data, msg->src);
		break;

	case pollMsg:
		if (msg->flags & MSG_FLAG_IDLE) {
			w = 500;
			msg->flags = 0;
		}
		poll(w);
		system->postMsg(msg);
		return;
	default:
		throw std::runtime_error("Unexpected message");
		break;
	}
	system->releaseMessage(msg);
}




void EpollTask::subscribe(const EpollMsgData *data, Task *src)
{
	int op;
	struct epoll_event event;
	//TODO: check if already subscribed
	if (data->fd >= MAX_FD_SIZE )
		throw std::runtime_error("EpollTask::subscribe fd size exceeded");
	if (fds[data->fd].fd == data->fd)
		op = EPOLL_CTL_MOD;
	else
		op = EPOLL_CTL_ADD;

	fds[data->fd].fd = data->fd;
	fds[data->fd].state = data->state;
	fds[data->fd].ops = data->info;
	fds[data->fd].task = src;
	fds[data->fd].autocreate = data->autocreate;
	std::cout << "subscribe msg=" << data->autocreate << std::endl;
	event.data.fd = data->fd;
	event.events = data->info;
	int s = epoll_ctl (efd, op, data->fd, &event);
	if (s == -1) {
	}
	std::cout << "EpollTask::subscribe " << src->getId() << " epoll events=" << event.events << std::endl;
}

void EpollTask::unSubscribe(const EpollMsgData *data, Task *src)
{
	struct epoll_event event;
	//TODO: check if already subscribed
	memset(&fds[data->fd], 0, sizeof(EpollFD));
	event.data.fd = data->fd;
	if (data->info) {
		event.events = data->info;
		int s = epoll_ctl (efd, EPOLL_CTL_DEL, data->fd, &event);
		if (s == -1) {
		}
	}
	if (data->autocreate) {
	    close(data->fd);
	}
	std::cout << "EpollTask::unSubscribe " << src->getId() << std::endl;
}

int EpollTask::acceptNewConnection(EpollFD *fd)
{
    int infd = fd->fd;
	if (fd->autocreate) {
	  struct sockaddr in_addr;
	  socklen_t in_len = sizeof(in_addr);
	  
	  infd = accept(fd->fd, &in_addr, &in_len);
	  if (infd == -1)
	    return EAGAIN;
	  make_socket_non_blocking (infd);
	  EpollMsgData data = {infd, fd->ops, fdstate_connected};
	  subscribe(&data, fd->task);
	  
	}
	Message *msg = prepareMsg(this, fd->task, connectedMsg, infd, fd->ops);
	system->postMsg(msg);

	return 0;
}

char *EpollTask::readBuffer(Task *srcTask, Task *epollTask, int fd, int *len)
{
	char *buffer = bufferPool.getBuffer();
	*len =  read(fd, buffer, MAX_BUFFER_SZ);
	if (*len < 0) {
		bufferPool.releaseBuffer(buffer);
		return NULL;
	}

	if (*len == 0) {
		close(fd);
		std::cout << "Closing socket=" << fd << std::endl;
		Message *msg = EpollTask::prepareMsg(srcTask, epollTask, unSubscribeMsg, fd, 0);
		srcTask->getSystem()->postMsg(msg);
		bufferPool.releaseBuffer(buffer);
		return NULL;
	}
	return buffer;
}

void EpollTask::poll(int w)
{
	int i, n;

	if (!(n = epoll_wait (efd, events, maxEvents, w)))
		return;
	struct epoll_event *event;

	n = std::min(n, maxPoll);

	for (i = 0; i < n; i++) {
		event = events + i;
        
		EpollFD *fd = fds + event->data.fd;

		//Error on socket
		if ((event->events & EPOLLERR) ||
              (event->events & EPOLLHUP)) {
			close(fd->fd);
			Message *msg = prepareMsg(this, fd->task, disconnectedMsg, fd->fd, fd->ops);
			std::cout << "Disconnected message on fd=" << event->data.fd << " fd saved=" << fd->fd << " event=" <<
			  event->events << std::endl;
			perror("event error");
			memset(&fds[fd->fd], 0, sizeof(EpollFD));
			system->postMsg(msg);
		}
		//Event on RX direction
		if (event->events & EPOLLIN) {
			if (fd->state == fdstate_connected) {
				Message *msg = prepareMsg(this, fd->task, readMsg, fd->fd, fd->ops);
				system->postMsg(msg);
			}
			else {// new connection
			  if (fd->fd > 0)
			    if (acceptNewConnection(fd) != EAGAIN)
			      std::cout << "Connected message on fd=" << event->data.fd << " fd saved=" << fd->fd << std::endl;
			}
		}
		//Event on TX direction
		if (event->events & EPOLLOUT) {
			if (fd->fd == 0)
				continue;
			MsgType msgType;
			if (fd->state == fdstate_connected) // notify that we can write again
				 msgType= writeMsg;
			else {
				fd->state = fdstate_connected;
				msgType = connectedMsg;
				std::cout << "POLLOUT Connected message on fd=" << event->data.fd << " fd saved=" << fd->fd << std::endl;
			}
			Message *msg = prepareMsg(this, fd->task, msgType, fd->fd, fd->ops);
			system->postMsg(msg);
		}
	}
}
