/*
 * pcapintf.h
 *
 *  Created on: Dec 30, 2016
 *      Author: awahl
 */

#ifndef PCAPINTF_H_
#define PCAPINTF_H_
#include <pcap.h>
#include <spdlog/spdlog.h>
#include <netinet/in.h>
#include "system.h"

class Packet {
    u_char key[128];
    int key_len;
    /* ip params */
    int pkt_len;
    int srcPort;
    int dstPort;
    int ipOffset, payloadOffset;
    int prot, iplen;
    int switchDirection;
    std::shared_ptr<spdlog::logger> logger;


public:
    Packet(const u_char *pkt, size_t len);
    void *getKey() { return key;}
    int getKeyLen() { return key_len;}
    int getPayloadOffset() {return payloadOffset;}

    int getSwitchDirection() const;
};


class PcapInterface : public Task {
	Task *eventTask;
	Task *kafkaProducer;
	pcap_t *pcap_hnd;
	int pcap_fd;
	char pcap_err_str[PCAP_ERRBUF_SIZE];
    std::shared_ptr<spdlog::logger> logger;


public:
	PcapInterface(System *system) : Task(system, "pcap_task") {
		logger = spdlog::get("logger");
	}

	~PcapInterface() {}
	void execute(Message *msg);
	void init(Message *msg);
    void processPacket(const struct pcap_pkthdr *hdr, const u_char *pkt);

};

extern BufferPool<u_char> packetPool;

#endif /* PCAPINTF_H_ */
