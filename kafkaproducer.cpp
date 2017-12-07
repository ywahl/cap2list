//
// Created by awahl on 11/30/17.
//
#include <cstdint>
#include <spdlog/spdlog.h>
#include "kafkaproducer.h"
#include "pcapintf.h"


static KafkaProducer *kProducer;

void KafkaMessage::init(const void *key, int k_len, const void *data, int d_len)
{
    key_len = k_len;
    data_len = d_len;
    memcpy(payload, key, k_len);
    memcpy((char *)payload + k_len, data, d_len);
}


void *KafkaMessage::init(const void *key, int k_len)
{
    key_len = k_len;
    data_len = 0;
    memcpy(payload, key, k_len);
    return payload + k_len;
}

void *KafkaMessage::addData(const void *data, int d_len)
{
    u_char *ptr = payload + key_len + data_len;
    memcpy(ptr, data, d_len);
    data_len += d_len;
    return payload + key_len + data_len;
}



void *KafkaMessage::getKey()
{
    return payload;
}


void *KafkaMessage::getData()
{
    return (void *) ((char *)payload + key_len);
}


static void dr_msg_cb (rd_kafka_t *rk,
                       const rd_kafka_message_t *rkmessage, void *opaque) {
    KafkaProducer *kafkaProducer = kProducer;
    kafkaProducer->rdKafkaCallback(rk, rkmessage);
}

void KafkaProducer::rdKafkaCallback(rd_kafka_t *rk, const rd_kafka_message_t *rkmessage)
{
    if (rkmessage->err)
        logger->error("Message delivery failed: {}", rd_kafka_err2str(rkmessage->err));
    else
        logger->info("Message delivered ({} bytes, partition {})",
                rkmessage->len, rkmessage->partition);
}



void KafkaProducer::init(Message *msg)
{
    kProducer = this;
    rd_kafka_conf_t *conf;
    char errstr[512];       /* librdkafka API error reporting buffer */
    logger = spdlog::get("logger");

    logger->info("KafkaProducer init topic={} broker={}", config.get("topic"), config.get("broker"));


    conf = rd_kafka_conf_new();

    if (rd_kafka_conf_set(conf, "bootstrap.servers", config.get("broker"),
                          errstr, sizeof(errstr)) != RD_KAFKA_CONF_OK) {
        logger->error("Error configuring kafkaproducer {}", errstr);
        return;
    }

    rd_kafka_conf_set_dr_msg_cb(conf, dr_msg_cb);

    rk = rd_kafka_new(RD_KAFKA_PRODUCER, conf, errstr, sizeof(errstr));
    if (!rk) {
        logger->error("Failed to create new producer: {}", errstr);
        return;
    }
    rkt = rd_kafka_topic_new(rk, config.get("topic"), NULL);
    if (!rkt) {
        logger->error("Failed to create topic object: {}", rd_kafka_err2str(rd_kafka_last_error()));
        rd_kafka_destroy(rk);
        return;
    }
    logger->info("Kafka producer initialized");
}


void KafkaProducer::send(Message *msg)
{
    KafkaMessage *kafkaMessage = static_cast<KafkaMessage *>(msg->data);
    logger->info("KafkaProducer key={} data={}", kafkaMessage->getKeyLen(),
                 kafkaMessage->getDataLen());

    retry:
    if (rd_kafka_produce(
            /* Topic object */
            rkt,
            /* Use builtin partitioner to select partition*/
            RD_KAFKA_PARTITION_UA,
            /* Make a copy of the payload. */
            RD_KAFKA_MSG_F_COPY,
            /* Message payload (value) and length */
            kafkaMessage->getData(), kafkaMessage->getDataLen(),
            /* Optional key and its length */
            kafkaMessage->getKey(), kafkaMessage->getKeyLen(),
            /* Message opaque, provided in
             * delivery report callback as
             * msg_opaque. */
            this) == -1) {
        /**
         * Failed to *enqueue* message for producing.
         */
         logger->error("Failed to produce to topic {}: {}", rd_kafka_topic_name(rkt),
                rd_kafka_err2str(rd_kafka_last_error()));

        /* Poll to handle delivery reports */
        if (rd_kafka_last_error() == RD_KAFKA_RESP_ERR__QUEUE_FULL) {
            /* If the internal queue is full, wait for
             * messages to be delivered and then retry.
             * The internal queue represents both
             * messages to be sent and messages that have
             * been sent or failed, awaiting their
             * delivery report callback to be called.
             *
             * The internal queue is limited by the
             * configuration property
             * queue.buffering.max.messages */
            rd_kafka_poll(rk, 1000/*block for max 1000ms*/);
            goto retry;
        }
    } else {
        logger->info("Enqueued message ({} bytes) for topic {} for this={}",
                kafkaMessage->getDataLen(), rd_kafka_topic_name(rkt), (u_long )this);
    }
    /* A producer application should continually serve
     * the delivery report queue by calling rd_kafka_poll()
     * at frequent intervals.
     * Either put the poll call in your main loop, or in a
     * dedicated thread, or call it after every
     * rd_kafka_produce() call.
     * Just make sure that rd_kafka_poll() is still called
     * during periods where you are not producing any messages
     * to make sure previously produced messages have their
     * delivery report callback served (and any other callbacks
     * you register). */
    rd_kafka_poll(rk, 0/*non-blocking*/);
}


void KafkaProducer::execute(Message *msg)
{
    int n;
    switch(msg->type) {
        case initMsg:
            init(msg);
            break;
        case readMsg:
            send(msg);
            packetPool.releaseBuffer((u_char *)msg->data);
            logger->info("releasing buffer {}", packetPool.getLength());
            break;
    }
    system->releaseMessage(msg);
}