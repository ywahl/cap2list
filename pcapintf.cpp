/*
 * pcapintf.cpp
 *
 *  Created on: Dec 31, 2016
 *      Author: awahl
 */

#include <iostream>
#include <algorithm>
#include <net/ethernet.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include "poll.h"
#include "pcapintf.h"
#include "kafkaproducer.h"



BufferPool<u_char> packetPool(NUM_BUFFERS, MAX_BUFFER_SZ, "BUFP");

Packet::Packet(const u_char *packet, size_t len)
{
    logger = spdlog::get("logger");
    logger->info("Initializing packet {}", len);

    pkt_len = len;
    struct tcphdr *tcp_hdr = NULL;
    struct udphdr *udp_hdr = NULL;

    const u_char *ptr;
    struct ether_header *ethhdr;

    switchDirection = 0;

    ptr = packet + sizeof(struct ether_header);
    ethhdr = (struct ether_header*) packet;
    key_len = -1;

    if (ntohs(ethhdr->ether_type) == ETHERTYPE_IP) {
        logger->info("IP Packet");
        ipOffset = sizeof(struct ether_header);

        struct iphdr *ip_hdr = (struct iphdr *) ptr;
        prot = ip_hdr->protocol & 0xff;
        iplen = ip_hdr->ihl * 4;
        payloadOffset = ipOffset + iplen;
        switch (prot) {
            case IPPROTO_TCP:
                tcp_hdr = (struct tcphdr *) (ptr + iplen);
                dstPort = ntohs(tcp_hdr->dest);
                srcPort = ntohs(tcp_hdr->source);
                if (dstPort > srcPort)
                    switchDirection = 1;
                payloadOffset += 4 * tcp_hdr->doff;
                logger->info("ipoffset={} iplen={} tcp doff={}", ipOffset, iplen, tcp_hdr->doff);
                break;
            case IPPROTO_UDP:
                udp_hdr = (struct udphdr *) (ptr + iplen);
                dstPort = ntohs(udp_hdr->dest);
                srcPort = ntohs(udp_hdr->source);
                if (dstPort > srcPort)
                    switchDirection = 1;
                payloadOffset += sizeof(struct udphdr);
                break;
        }
        if (switchDirection) {
            u_int32_t tmpAddr = ip_hdr->daddr;
            ip_hdr->daddr = ip_hdr->saddr;
            ip_hdr->saddr = tmpAddr;
            int tmpPort = dstPort;
            dstPort = srcPort;
            srcPort = tmpPort;
        }
        //5 Tuple key
        u_char *ptr = key;
        memcpy(ptr, &ip_hdr->daddr, sizeof(ip_hdr->daddr));
        ptr += sizeof(ip_hdr->daddr);
        memcpy(ptr, &ip_hdr->saddr, sizeof(ip_hdr->saddr));
        ptr += sizeof(ip_hdr->saddr);
        memcpy(ptr, &prot,  sizeof(prot));
        ptr += sizeof(prot);
        memcpy(ptr, &dstPort, sizeof(dstPort));
        ptr += sizeof(dstPort);
        memcpy(ptr, &srcPort, sizeof(srcPort));
        key_len = sizeof(ip_hdr->daddr) +
                  sizeof(ip_hdr->saddr) +
                  sizeof(prot) +
                  sizeof(dstPort) +
                  sizeof(srcPort);
        logger->info("IP packet payload offset={}", payloadOffset);
    }
    else
        logger->info("Non-IP packet {}", ntohs(ethhdr->ether_type));
}

int Packet::getSwitchDirection() const {
    return switchDirection;
}


void PcapInterface::init(Message *msg)
{
	const char *dev = config.get("dev");
    const char *filter = config.get("filter");
	pcap_hnd = pcap_create(dev, pcap_err_str);
    logger->info("pcapInterface dev={} filter={}", dev, filter);
	//if (pcap_set_promisc(cfg.pcap, 1))              {fprintf(stderr,"pcap_set_promisc failed\n"); goto done;}
	//if (pcap_set_snaplen(cfg.pcap, cfg.snaplen))    {fprintf(stderr,"pcap_set_snaplen failed\n"); goto done;}
	//if (pcap_set_buffer_size(cfg.pcap, cfg.capbuf)) {fprintf(stderr,"pcap_set_buf_size failed\n");goto done;}
	int ret = pcap_activate(pcap_hnd);
    if (filter != nullptr) {

        struct bpf_program fp;
        bpf_u_int32 mask;		/* Our netmask */
        bpf_u_int32 net;		/* Our IP */

        /* ask pcap for the network address and mask of the device */
        pcap_lookupnet(dev, &net, &mask, pcap_err_str);
        if (pcap_compile(pcap_hnd, &fp, filter, 0, mask) == -1) {
            logger->error("Error compiling pcap filter {} on dev {}", filter, dev);
            return;
        }

        if (pcap_setfilter(pcap_hnd, &fp) == -1) {
            logger->error("Error setting pcap filter {} on dev {}", filter, dev);
            return;
        }
    }
	pcap_fd = pcap_get_selectable_fd(pcap_hnd);
	logger->info("Pcap activating fd {} ret={}", pcap_fd, ret);
	eventTask = system->getTask("epollTask");
	kafkaProducer = system->getTask("KafkaProducerTask");
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
    int len = (int)std::min<int>(hdr->len, 1600);
    logger->info("processPacket {}", len);
    Packet packet(pkt, len);
    if (packet.getKeyLen() == -1)
        return;
    Message *message = system->getMessage();
    message->src = this;
    message->dst = kafkaProducer;
    message->data = packetPool.getBuffer();
    message->type = readMsg;

    KafkaMessage *kafkaMessage = (KafkaMessage *)(message->data);
    kafkaMessage->init(packet.getKey(), packet.getKeyLen());
    int tmp = packet.getSwitchDirection();
    kafkaMessage->addData(&hdr->ts, sizeof(timeval));
    kafkaMessage->addData(&tmp, sizeof(tmp));
    tmp = packet.getPayloadOffset();
    kafkaMessage->addData(&tmp, sizeof(int));
    kafkaMessage->addData(pkt, len);
    system->postMsg(message);
}


void PcapInterface::execute(Message *msg)
{
	int n;
    logger->info("PcapInterface msg received");
	switch(msg->type) {
	case initMsg:
		init(msg);
		break;
	case readMsg:
		logger->debug("pkt received {}");
		n = pcap_dispatch(pcap_hnd, 10, cb, (u_char *)this);
		if ( n < 0) {
		   //pcap_perror(cfg.pcap, "pcap error: ");
		}
		else
			logger->debug("#pkts processed={}", n);
		break;
	}
	system->releaseMessage(msg);
}

