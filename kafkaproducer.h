//
// Created by awahl on 11/30/17.
//

#ifndef CAP2LIST_KAFKAPRODUCER_H
#define CAP2LIST_KAFKAPRODUCER_H


#include <spdlog/logger.h>
#include <librdkafka/rdkafka.h>
#include "system.h"

class KafkaMessage {
    size_t key_len;
    size_t data_len;
    u_char  payload[1];
public:
    void init(const void *key, int key_len,
                 const void *data, int data_len);
    void *init(const void *key, int key_len);
    void *addData(const void *data, int data_len);
    void *getKey();
    size_t getKeyLen() { return  key_len;}
    void *getData();
    size_t getDataLen() { return data_len;}
};

class KafkaProducer: public Task {
    std::shared_ptr<spdlog::logger> logger;
    rd_kafka_t *rk;         /* Producer instance handle */
    rd_kafka_topic_t *rkt;
public:
    KafkaProducer(System *system) : Task(system, "KafkaProducerTask") {}
    ~KafkaProducer() {}
    void execute(Message *msg);
    void init(Message *msg);
    void send(Message *msg);
    void rdKafkaCallback(rd_kafka_t *rk, const rd_kafka_message_t *rkmessage);
};


#endif //CAP2LIST_KAFKAPRODUCER_H
