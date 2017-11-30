/*
 * pcapintf.cpp
 *
 *  Created on: Dec 31, 2016
 *      Author: awahl
 */

#include <iostream>
#include "poll.h"
#include "pcapintf.h"


void PcapInterface::init(Message *msg)
{
	const char *dev = config.get("dev");
    logger = spdlog::get("logger");
	pcap_hnd = pcap_create(dev, pcap_err_str);
    logger->info("pcapInterface dev={}", dev);
	//if (pcap_set_promisc(cfg.pcap, 1))              {fprintf(stderr,"pcap_set_promisc failed\n"); goto done;}
	//if (pcap_set_snaplen(cfg.pcap, cfg.snaplen))    {fprintf(stderr,"pcap_set_snaplen failed\n"); goto done;}
	//if (pcap_set_buffer_size(cfg.pcap, cfg.capbuf)) {fprintf(stderr,"pcap_set_buf_size failed\n");goto done;}
	int ret = pcap_activate(pcap_hnd);
	pcap_fd = pcap_get_selectable_fd(pcap_hnd);
	std::cout << "Pcap activsting fd="  << pcap_fd << " ret=" << ret << std::endl;
	logger->info("Pcap activating fd {} ret={}", pcap_fd, ret);
	eventTask = system->getTask("epollTask");
	Message *m = EpollTask::prepareMsg(this, eventTask, subscribeMsg, pcap_fd ,EPOLLIN);
	EpollMsgData *data = reinterpret_cast<EpollMsgData *>(m->smallData);
	data->state = fdstate_connected;

	getSystem()->postMsg(m);
}

void cb(u_char *data, const struct pcap_pkthdr *hdr, const u_char *pkt)
{

	PcapInterface *pcapIntf = reinterpret_cast<PcapInterface *>(data);

    pcapIntf->processPacket(hdr, pkt);
}

void PcapInterface::processPacket(const struct pcap_pkthdr *hdr, const u_char *pkt)
{
    logger->info("processPacket {}", hdr->len)
}

void PcapInterface::execute(Message *msg)
{
	int n;
	switch(msg->type) {
	case initMsg:
		init(msg);
		break;
	case readMsg:
		logger->debug("pkt received {}");
		n = pcap_dispatch(pcap_hnd, 10000, cb, (u_char *)this);
		if ( n < 0) {
		   //pcap_perror(cfg.pcap, "pcap error: ");
		}
		else
			logger->debug("#pkts processed={}", n);l
		break;
	}
	system->releaseMessage(msg);
}

