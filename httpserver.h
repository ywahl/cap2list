//
// Created by awahl on 3/7/20.
//

#ifndef CAP2LIST_HTTPSERVER_H
#define CAP2LIST_HTTPSERVER_H
#include <map>
#include <spdlog/spdlog.h>
#include "sockets.h"

class HttpServer : public Task {
    std::shared_ptr<spdlog::logger> logger;
    TcpSocket socket;
    std::map<int, TcpSocket *> fd2Socket;
public:
    HttpServer(System *system) : Task(system, "HttpServerTask"), socket(this) {
        logger = spdlog::get("logger");
    }
    ~HttpServer() {}
    void execute(Message *msg);
    void init(Message *msg);
    void send(Message *msg);
    void read(Message *msg, int s);
};

#endif //CAP2LIST_HTTPSERVER_H
