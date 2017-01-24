/*
 * sockets.h
 *
 *  Created on: Oct 2, 2014
 *      Author: awahl
 */

#ifndef SOCKETS_H_
#define SOCKETS_H_

#include <sys/types.h>
#include <sys/socket.h>


#include "queue.h"


struct SocketInitData {
	Task *endPoint;
	char ipaddr[20];
	short port;
};

#define TCP_SOCKET_DEFAULT_PORT 55555

enum SocketRxTxState {
	sockRxTxActive,
	sockRxTxStopped,
};

class TcpSocket : public virtual Object {
	int s;
	bool srv;
	Task *eventTask;
	Task *parentTask;
	TcpSocket *parentSocket;
	SocketRxTxState txState, rxState;

	char *txStopPtr;
	int txStopLen;

public:
 TcpSocket(Task *p) : s(-1), srv(false), eventTask(NULL), parentTask(p), parentSocket(NULL), txState(sockRxTxStopped), rxState(sockRxTxStopped) {}
        TcpSocket(TcpSocket *parentSocket, int p);
	~TcpSocket();

	int sendBuffer(char *buf, int len);
	void server(const char *bindAddr, int port);
	void client(const char *clientAddr, int port);
	int setsockopt(int level, int optname, const void *optval, socklen_t optlen);
	int getsockopt(int level, int optname, void *optval, socklen_t *optlen);

	//Handlers do not release or acquire resources
	void init(Message *);
	void processConnectMsg(Message *);
	void processDisconnectMsg(Message *);
	void processReadMsg(Message *);
	void processWriteMsg(Message *);
};

#endif /* SOCKETS_H_ */
