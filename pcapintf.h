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
#include "system.h"


class PcapInterface : public Task {
	Task *eventTask;
	pcap_t *pcap_hnd;
	int pcap_fd;
	char pcap_err_str[PCAP_ERRBUF_SIZE];
    std::shared_ptr<spdlog::logger> logger;

public:
	PcapInterface(System *system) : Task(system, "pcap_task") {}
	~PcapInterface() {}
	void execute(Message *msg);
	void init(Message *msg);
    void processPacket(const struct pcap_pkthdr *hdr, const u_char *pkt);

};


#endif /* PCAPINTF_H_ */
