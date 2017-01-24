#include <iostream>
#include <unistd.h>
#include "system.h"
#include "queue.h"


ExecutorAction Task::executeAction(Message *msg)
{
	MsgType type = msg->type;
	ExecutorAction action;
	Task *t;
	for (int i = 0; i < executors[type].size(); i++) {
		Executor ex = (executors[type])[i];
		action = (t->*ex)(msg);
		if (action != execCont)
			return action;
	}
	return execConsumed;
}


void Scheduler::postMsg(Message *m)
{
	if (m->dst == NULL)
		throw std::runtime_error("Message dst == NULL");
	msgQ.push(m);
}

Message *Scheduler::getMsg()
{
	int sz = msgQ.size();
	if (sz > 1) {
		Message *msg = msgQ.front();
		msgQ.pop();
		return msg;
	}
	if (!sz)
		return &idleMessage;
	Message *msg = msgQ.front();
	msgQ.pop();
	if (msg->dst == idleTask)
		msg->flags |= MSG_FLAG_IDLE;
	return msg;
}


void Scheduler::execute(Message *)
{
	sleep(1);
}

void Thread::loop()
{
	Message *msg, *nextMsg;
	int i = 0;


	while(!stop) {
		i++;
		msg = scheduler->getMsg();
		nextMsg = msg->nextMsg;
		msg->dst->execute(msg);

		//Message linking
		if (nextMsg)
			scheduler->postMsg(nextMsg);

//		if (!(i % 10000)) {
//			std::cout << "Scheduler QSize: " << scheduler->getAmountOfWork() << std::endl;
//		}

	}
}

volatile bool Thread::stop = false;



void TaskList::execute(Message *msg)
{
	Task *t = taskList[curWorkItem];
	t->execute(msg);

	curWorkItem++;
	if (curWorkItem < taskList.size()) {
		system->postMsg(msg);
	}
}


void TaskList::addTask(Task *t)
{
	taskList.push_back(t);
}

Task *BasicSystem::getTask(const char *name)
{
	for (auto task : taskList) {
		if (task->getId().compare(name) == 0)
			return task;
	}
	return NULL;
}
