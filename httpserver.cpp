//
// Created by awahl on 3/7/20.
//
#include "httpserver.h"
#include "poll.h"

void HttpServer::execute(Message *msg) {
    int n;
    EpollMsgData *data = reinterpret_cast<EpollMsgData *>(msg->smallData);
    switch(msg->type) {
        case initMsg:
            init(msg);
            break;
        case readMsg:
            read(msg, data->fd);
            break;
        case connectedMsg:
            {
                TcpSocket *client = socket.processConnectMsg(msg);
                fd2Socket[data->fd] = client;
            }
            break;
        case disconnectedMsg:
            fd2Socket.erase(data->fd);
            logger->info("client socket closed {}", data->fd);
            break;
        default:
            logger->error("unsupported message");
    }
    system->releaseMessage(msg);
}


void HttpServer::init(Message *msg) {
    logger->info("initialization httpserver");
    config.set("server", "true");
    config.set("ipaddress", "0.0.0.0");
    config.set("port", "8080");
    socket.init(msg);
}

void HttpServer::read(Message *msg, int s) {
    int len;
    TcpSocket *client = fd2Socket[s];
    if (client == nullptr) {
        logger->error("no mapping from fd to socket");
        return;
    }
    char *buffer = client->processReadMsg(msg, &len);
    if (buffer == nullptr) {
        logger->info("readMsg nullPtr");
        return;
    }
    //logger->info("read {} {}", len, buffer);
    bufferPool.releaseBuffer(buffer);
    const char *ok = "HTTP/1.1 200 OK\r\nConnection: close\r\n\r\n";
    client->sendBuffer((void *)ok, strlen(ok));
    fd2Socket.erase(s);
    Message *discMsg = EpollTask::prepareMsg(this, system->getTask("epollTask"),
            unSubscribeMsg, s, 0, true);
    system->postMsg(discMsg);

}