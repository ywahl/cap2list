/*
 * sockets.cpp
 *
 *  Created on: Oct 2, 2014
 *      Author: awahl
 */


#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <iostream>


#include "poll.h"
#include "sockets.h"


TcpSocket::TcpSocket(TcpSocket *parentSocket, int p)
{
  s = p;
  srv = false;
  eventTask = parentSocket->eventTask;
  parentTask = parentSocket->parentTask;
  this->parentSocket = parentSocket;
  rxState = sockRxTxActive;
  txState = sockRxTxActive;
}


TcpSocket::~TcpSocket()
{
	if (s != -1)
		close(s);
}

void TcpSocket::server(const char *bindAddr, int port)
{
	struct sockaddr_in sa;
	s = socket (AF_INET, SOCK_STREAM, 0);

	memset(&sa, 0, sizeof(sa));
	sa.sin_family = AF_INET;
	if (bindAddr)
		inet_aton(bindAddr, &sa.sin_addr);
	else
		sa.sin_addr.s_addr = INADDR_ANY;
	sa.sin_port = htons(port);

	if (bind (s, (struct sockaddr *)&sa, sizeof(sa)) < 0)
		throw std::runtime_error("cannot bind socket to address");

	make_socket_non_blocking(s);
	if (listen (s, SOMAXCONN) < 0)
		throw std::runtime_error("cannot list socket to address");

	Message *msg = EpollTask::prepareMsg(parentTask, eventTask, subscribeMsg, s ,EPOLLIN, true);
	parentTask->getSystem()->postMsg(msg);
}

void TcpSocket::client(const char *clientAddr, int port)
{
	if (clientAddr == NULL)
		throw std::runtime_error("server Addr is NULL");

	struct sockaddr_in sa;

	memset(&sa, 0, sizeof(sockaddr_in));

	if (!inet_aton(clientAddr, &sa.sin_addr)) {
				struct hostent *he;

				if ((he = gethostbyname(clientAddr)) == NULL) {
					fprintf(stderr,"[NetTools] Invalid server name: %s\n",clientAddr);
					throw std::runtime_error("invalid server name");
				}
				else
					sa.sin_addr = *(struct in_addr *) *(he->h_addr_list);
	}
	sa.sin_port = htons(port);
	sa.sin_family = AF_INET;

	s = socket(AF_INET, SOCK_STREAM, 0);
	make_socket_non_blocking(s);

	if (s < 0)
		throw std::runtime_error("Could not get socket");

	connect(s, (struct sockaddr *)&sa, sizeof(struct sockaddr_in));
	// Note the following works only because this is single threaded
	Message *msg = EpollTask::prepareMsg(parentTask, eventTask, subscribeMsg, s ,EPOLLOUT | EPOLLONESHOT);
	parentTask->getSystem()->postMsg(msg);
}

void TcpSocket::init(Message *msg)
{
	srv = parentTask->config.get("server", true);

	const char *ipaddr = parentTask->config.get("ipaddress");
	int port = parentTask->config.get("port", TCP_SOCKET_DEFAULT_PORT);

	if (srv)
		return server(ipaddr, port);

	return client(ipaddr, port);
}

void TcpSocket::processConnectMsg(Message *msg)
{
	if (srv) {
		std::cout << "Srv Connected" << std::endl;
	}
	txState = rxState = sockRxTxActive;
}

void TcpSocket::processDisconnectMsg(Message *msg)
{

}

void TcpSocket::processReadMsg(Message *msg)
{

}

//Write Q was full. The event Task sends a write msg to notify us that we can start sending
//again
void TcpSocket::processWriteMsg(Message *msg)
{
	if (txState == sockRxTxActive) {
		std::cout << "double writeMsg" << std::endl;
		return;
	}
	std::cout << "processWriteMsg ptr=" << (void *)txStopPtr << " txStopLen=" << txStopLen << std::endl;
	//Disable EPOLLOUT, register only for RX in the eventTask
	Message *msgPoll = EpollTask::prepareMsg(parentTask, eventTask, subscribeMsg,	s, EPOLLIN);
	EpollMsgData *data = reinterpret_cast<EpollMsgData *>(msgPoll->smallData);
	data->state = fdstate_connected;
	parentTask->system->postMsg(msgPoll);
	txState = sockRxTxActive;
	sendBuffer(txStopPtr, txStopLen);
}

int TcpSocket::sendBuffer(char *buf, int len)
{      
  int wlen = 0;
  if (txState == sockRxTxActive) {
    wlen = write(s, buf, len);
    if (len != wlen) {

      txStopPtr = buf + wlen;
      txStopLen = len - wlen;
      std::cerr << "write Q full len=" << len << " wlen=" << wlen << " txStopLen=" << txStopLen <<  std::endl;
      txState = sockRxTxStopped;
      //Subscribe to EPOLLOUT in addition to EPOLLIN
      Message * msg = EpollTask::prepareMsg(parentTask, eventTask, subscribeMsg, s, EPOLLOUT | EPOLLIN);
      EpollMsgData *data = reinterpret_cast<EpollMsgData *>(msg->smallData);
      data->state = fdstate_connected;
      parentTask->system->postMsg(msg);
    }
  }
  return wlen;
}
