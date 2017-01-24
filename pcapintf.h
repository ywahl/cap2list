/*
 * pcapintf.h
 *
 *  Created on: Dec 30, 2016
 *      Author: awahl
 */

#ifndef PCAPINTF_H_
#define PCAPINTF_H_
#include <pcap.h>
#include "system.h"


class PcapInterface : public Task {
	Task *eventTask;
	pcap_t *pcap_hnd;
	int pcap_fd;
	char pcap_err_str[PCAP_ERRBUF_SIZE];

public:
	PcapInterface(System *system) : Task(system, "pcap_task") {}
	~PcapInterface() {}
	 void execute(Message *msg);
	 void init(Message *msg);
};


#endif /* PCAPINTF_H_ */
