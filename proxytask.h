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
	int idx;
	int rx;
	int state;
	TcpSocket *clients;
public:
	 ProxyTask(int portOffset, System *s, EpollTask *evtTsk) : Task(s, "ProxyTask"), TcpSocket(this, evtTsk), idx(portOffset), rx(0) {}
	  ~ProxyTask() {}
	  void execute(Message *msg);
private:
	  void processConnectMsg(Message *msg);
	  void initMultiThreadSystem(Message *msg);
};



struct ProxyMessage {
	uint32_t preamble;
	uint32_t length;
	MsgType type;
	uint32_t subtype;
	uint32_t flags;

	char smallData[32];
	char data[4];

	inline void init() { preamble = 0xABBABABA; }
} __attribute__((packed));



#endif /* PROXYTASK_H_ */
