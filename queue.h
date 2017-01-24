/*
 * queue.h
 *
 *  Created on: Sep 15, 2014
 *      Author: awahl
 */

#ifndef QUEUE_H_
#define QUEUE_H_
#include <queue>
#include <vector>
#include <string>
#include <stdexcept>
#include <memory.h>
#include "system.h"



class Scheduler;


//tm_value.it_value.tv_sec = 1;
//tm_value.it_interval.tv_sec = 1;



class Scheduler : public Task {
	Task *idleTask;
	Message idleMessage;
	//TODO don't use STL use static ring buffer
	std::queue<Message *> msgQ;
public:
	Scheduler(System *s) : Task(s, "Scheduler"), idleTask(this) {
		memset(&idleMessage, 0, sizeof(Message));
		idleMessage.dst = idleTask;
	}
	int getAmountOfWork() { return msgQ.size();}
	void postMsg(Message *);
	Message* getMsg();
	void execute(Message *);

	void setIdleTask(Task* i) {
		idleTask = i;
		idleMessage.dst = idleTask;
		idleMessage.type = pollMsg;
	}
};


class Thread : public Object {
	Scheduler *scheduler;
	static volatile bool stop;
public:
	Thread(Scheduler *sched) : scheduler(sched) {}
	void loop();
};


class TaskList : public Task {
	std::vector<Task *> taskList;
	bool consecutif;
	unsigned curWorkItem;
public:
	TaskList(System *s, const char *n, bool c) : Task(s, n), consecutif(c) { reset();}
	void reset() {curWorkItem = 0;}
	void addTask(Task *);
	void execute(Message *);
};


class BasicSystem : public System {
private:
	std::vector<Task *> taskList;
	BufferPool<Message> msgPool;
	Scheduler scheduler;

public:
	BasicSystem(int numOfMsg) : msgPool(numOfMsg, sizeof(Message), "MSGP"), scheduler(this)  {}
	void postMsg(Message *d) {
		scheduler.postMsg(d);
	}

	void registerTask(Task *t) {
		taskList.push_back(t);
	}

	Message *getMessage() {
		Message *msg = msgPool.getBuffer();
		msg->flags = 0;
		msg->nextMsg = NULL;
		return msg;
	}

	void releaseMessage(Message *msg) {
		msgPool.releaseBuffer(msg);
	}

	Scheduler *getScheduler() { return &scheduler;}

	Task *getTask(const char *name);
};


#endif /* QUEUE_H_ */
