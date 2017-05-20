/*
 * proxytask.cpp
 *
 *  Created on: Jan 27, 2017
 *      Author: awahl
 */
#include "spdlog/spdlog.h"
#include <iostream>
#include <thread>
#include "queue.h"
#include "poll.h"
#include "proxytask.h"
#include "multhreadedsystem.h"





void ProxyTask::processConnectMsg(Message *msg)
{
  EpollMsgData *data = reinterpret_cast<EpollMsgData *>(msg->smallData);
  struct sockaddr in_addr;
  socklen_t in_len = sizeof(in_addr);
  int infd = data->fd;
  //int infd = accept(data->fd, &in_addr, &in_len);
  //if (infd == -1)
  //  return;

  TcpSocket *serverSocket = new TcpSocket(static_cast<TcpSocket *>(this), infd);
  serverMap[infd] = serverSocket;
  MultiThreadedSystem::getLogger()->info("Proxy task id={} Server client connected: {}", idx, infd);
}


void ProxyTask::initMultiThreadSystem(Message *msg)
{
	//Check if master task
	if (idx == 0)
		return;
	clients = new TcpSocket(this , eventTask);
	config.unset("server");
	config.set("ipaddress", "127.0.0.1");
	config.set("port", "10000");
	clients->init(msg);
}


void ProxyTask::execute(Message *msg)
{
  switch(msg->type) {
  case initMsg:
    init(msg);
    initMultiThreadSystem(msg);
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
      //      if ((rx & 0xffff) == 0)
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


