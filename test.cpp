/*
 * test.cpp
 *
 *  Created on: May 11, 2015
 *      Author: awahl
 */
#include <iostream>
#include <map>
#include "test.h"
#include "poll.h"

void TestServer::processConnectMsg(Message *msg)
{
  EpollMsgData *data = reinterpret_cast<EpollMsgData *>(msg->smallData);
  struct sockaddr in_addr;
  socklen_t in_len = sizeof(in_addr);
  
  int infd = accept(data->fd, &in_addr, &in_len);
  if (infd == -1)
    return;
  
  close(infd);
  return;
  TcpSocket *serverSocket = new TcpSocket(static_cast<TcpSocket *>(this), infd);
  serverMap[infd] = serverSocket;
  std::cout << "Server client connected" << std::endl;
  
}



void TestServer::execute(Message *msg)
{
  switch(msg->type) {
  case initMsg:
    init(msg);
    break;
  case connectedMsg:
    processConnectMsg(msg);
    break;
  case readMsg:
    {
      int len;
      EpollMsgData *data = reinterpret_cast<EpollMsgData *>(msg->smallData);
      char *buffer = EpollTask::readBuffer(this, msg->src, data->fd, &len);
      if (buffer == NULL)
	break;
      rx++;
      if ((rx & 0xffff) == 0)
	std::cout << "rx = " << rx << std::endl;
	
      TcpSocket *peer = serverMap[data->fd];
      len = peer->sendBuffer(buffer, len);
      bufferPool.releaseBuffer(buffer);
    }
    break;

  case writeMsg:
    {
      EpollMsgData *data = reinterpret_cast<EpollMsgData *>(msg->smallData);
      TcpSocket *peer = serverMap[data->fd];
      peer->processWriteMsg(msg);
      std::cout << "processWriteMsg" << std::endl;
    }
    break;

  default:
    //throw std::runtime_error("Unexpected message");
    std::cout << "Unexpected message " << msg->type << std::endl;
    break;
  }
  system->releaseMessage(msg);
}



void TestClient::processConnectMsg(Message *msg)
{
  TcpSocket::processConnectMsg(msg);  
  std::cout << "Client connected" << std::endl;
  std::cout << "Sendbuffer=" << sendBuffer(txBuf, 1024) << std::endl;

  EpollMsgData *data = reinterpret_cast<EpollMsgData *>(msg->smallData);

  //Register for reads
  data->info = EPOLLIN;
  data->state = fdstate_connected;
  msg->type = subscribeMsg;
  msg->dst = msg->src;
  msg->src = this;
  system->postMsg(msg);
}


void TestClient::execute(Message *msg)
{
  switch(msg->type) {
  case initMsg:
    init(msg);
    break;

  case connectedMsg:
    // function processConnectMsg consumes messages
    return processConnectMsg(msg);

  case readMsg:
    {
      int len;
      EpollMsgData *data = reinterpret_cast<EpollMsgData *>(msg->smallData);
      char *buffer = EpollTask::readBuffer(this, msg->src, data->fd, &len);
      if (buffer == NULL)
	break;
      sendBuffer(buffer, len);
      sendBuffer(buffer, len);
      bufferPool.releaseBuffer(buffer);
    }
    break;

  case writeMsg:
    processWriteMsg(msg);
    break;

  default:
    //throw std::runtime_error("Unexpected message");
    std::cout << "Unexpected message " << msg->type << std::endl;
    break;
  }
  system->releaseMessage(msg);
}

