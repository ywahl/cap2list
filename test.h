/*
 * test.h
 *
 *  Created on: May 12, 2015
 *      Author: awahl
 */

#ifndef TEST_H_
#define TEST_H_

#include <map>
#include "system.h"
#include "sockets.h"


class TestServer : public Task, public TcpSocket {
  std::map<int, TcpSocket *> serverMap;
  int rx;
public:
 TestServer(System *s) : Task(s, "test_task"), TcpSocket(this), rx(0) {}
  ~TestServer() {}
  void execute(Message *msg);
  
  void processConnectMsg(Message *msg);
};

class TestClient : public Task, public TcpSocket {
  char txBuf[1024];
public:
 TestClient(System *s) : Task(s, "test_task"), TcpSocket(this) {}
  ~TestClient() {}
  void execute(Message *msg);
  void processConnectMsg(Message *msg);
};




#endif /* TEST_H_ */
