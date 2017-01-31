/*
 * proxytask.h
 *
 *  Created on: Jan 27, 2017
 *      Author: awahl
 */

#ifndef PROXYTASK_H_
#define PROXYTASK_H_
#include <map>
#include "system.h"
#include "sockets.h"


class ProxyTask : public Task, public TcpSocket {
	std::map<int, TcpSocket *> serverMap;
	int rx;
public:
	 ProxyTask(int portOffset, System *s, EpollTask *evtTsk) : Task(s, "ProxyTask"), TcpSocket(this, evtTsk), rx(0) {}
	  ~ProxyTask() {}
	  void execute(Message *msg);

	  void processConnectMsg(Message *msg);



};



#endif /* PROXYTASK_H_ */
