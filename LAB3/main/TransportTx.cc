#ifndef TRANSPORTTX
#define TRANSPORTTX

#include <string.h>
#include <omnetpp.h>

#include "FeedBackPacket_m.h"


using namespace omnetpp;

class TransportTx: public cSimpleModule {
private:
    cQueue buffer;
    cMessage *endServiceEvent;
    simtime_t serviceTime;
    int incrementServiceTime;            // How much increment serviceTime
    cOutVector bufferSizeVector;
    cOutVector packetDropVector;
public:
    TransportTx();
    virtual ~TransportTx();
protected:
    virtual void initialize();
    virtual void finish();
    virtual void handleMessage(cMessage *msg);
};

Define_Module(TransportTx);

TransportTx::TransportTx() {
    endServiceEvent = NULL;
}

TransportTx::~TransportTx() {
    cancelAndDelete(endServiceEvent);
}

void TransportTx::initialize() {
    buffer.setName("buffer");
    incrementServiceTime = 1;         // Initialized with 1 because is a multiplier
    endServiceEvent = new cMessage("endService");
    bufferSizeVector.setName("Buffer Size");
    packetDropVector.setName("Packet drops");
}

void TransportTx::finish() {
}

void TransportTx::handleMessage(cMessage *msg) {
    // msg is a packet
    if (msg->getKind() == 2) {
        // msg is a FeedBackPkt
        FeedBackPacket* feedbackPkt = (FeedBackPacket*) msg;

        // get the value of incrementServiceTime
         incrementServiceTime = feedbackPkt->getIncrementServiceTime();

        delete(msg);

    } else if (msg->getKind() == 0) {
        // msg is a data packet
        // if msg is signaling an endServiceEvent
        if (msg == endServiceEvent) {
            // if packet in buffer, send next one
            if (!buffer.isEmpty()) {
                // dequeue packet
                cPacket *pkt = (cPacket*) buffer.pop();
                // send packet
                send(pkt, "toOut$o");

                // start new service
                serviceTime = pkt->getDuration();
                // serviceTime multiplied by incrementServiceTime
                scheduleAt(simTime() + serviceTime * incrementServiceTime, endServiceEvent);

            }
        // check buffer limit
        } else if (buffer.getLength() >= par("bufferSize").intValue()) {
            // drop the packet
            delete msg;
            this->bubble("packet dropped");
            packetDropVector.record(1);
        } else {
            // enqueue the packet
            buffer.insert(msg);
            bufferSizeVector.record(buffer.getLength());
            // if the server is idle
            if (!endServiceEvent->isScheduled()) {
                // start the service now
                scheduleAt(simTime() + 0, endServiceEvent);
            }
        }
    }
}

#endif /* TransportTx */
