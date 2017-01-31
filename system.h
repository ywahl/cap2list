/*
 * system.h
 *
 *  Created on: Nov 30, 2014
 *      Author: awahl
 */

#ifndef SYSTEM_H_
#define SYSTEM_H_
#include "base.h"
#include "config.h"


class System : public Object {
public:
	virtual void postMsg(Message *d) = 0;
	virtual void registerTask(Task *) = 0;
	virtual ~System() {};
	virtual Message *getMessage() = 0;
	virtual void releaseMessage(Message *msg) = 0;
	//TODO refactor code to use getTask by name
	virtual Task *getTask(const char *name) = 0;
};


class Task;

enum ExecutorAction {
	execCont,
	execConsumed,
	execError
};


typedef ExecutorAction (Task::*Executor)(Message *);



class Task : public virtual Object {
protected:
	std::string id;
	std::vector<Executor> executors[lastMsg];

public:
	System *system;
	Configuration config;

	Task(System *s, const char *n) : system(s), id(n) { system->registerTask(this);};
	Task(const char *n) : system(NULL), id(n) {}

	virtual void execute(Message *) = 0;
	ExecutorAction executeAction(Message *msg);

	virtual ~Task() {
	}

	const std::string& getId() const {
		return id;
	}


	System* getSystem() const { return system;}

	//TODO refactor code to use this getMessage
	Message *getMessage(Task *d, MsgType t) {
		Message *msg = system->getMessage();
		msg->src = this;
		msg->dst = d;
		msg->type = t;
		return msg;
	}
};



#endif /* SYSTEM_H_ */
