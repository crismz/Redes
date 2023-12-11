#ifndef QUEUE
#define QUEUE

#include <string.h>
#include <omnetpp.h>

#include "FeedBackPacket_m.h"

using namespace omnetpp;

class Queue: public cSimpleModule {
private:
    cQueue buffer;
    cQueue bufferFeedPacket;
    int packetReceived;         // Counter
    int incrementServiceTime;   // How much increment serviceTime
    cMessage *endServiceEvent;
    simtime_t serviceTime;
    cOutVector bufferSizeVector;
    cOutVector packetDropVector;
public:
    Queue();
    virtual ~Queue();
protected:
    virtual void initialize();
    virtual void finish();
    virtual void handleMessage(cMessage *msg);
};

Define_Module(Queue);

Queue::Queue() {
    endServiceEvent = NULL;
}

Queue::~Queue() {
    cancelAndDelete(endServiceEvent);
}

void Queue::initialize() {
    buffer.setName("buffer");
    packetReceived = 0;
    incrementServiceTime = 1;       // Initialized with 1 because is a multiplier
    bufferFeedPacket.setName("bufferFeedPacket");
    endServiceEvent = new cMessage("endService");
    bufferSizeVector.setName("Buffer Size");
    packetDropVector.setName("Packet drops");
}

void Queue::finish() {
}

void Queue::handleMessage(cMessage *msg) {

    // if msg is signaling an endServiceEvent
    if (msg == endServiceEvent) {
        // if packet in bufferFeed
        if (!bufferFeedPacket.isEmpty()) {
            // dequeue packet
            FeedBackPacket *feedBackPkt = (FeedBackPacket*) bufferFeedPacket.pop();
            // send packet to queue2
            send(feedBackPkt, "out");
            // Start new service
            serviceTime = feedBackPkt->getDuration();
            scheduleAt(simTime() + serviceTime, endServiceEvent);

        // if packet in buffer, send next one
        } else if (!buffer.isEmpty()) {
            // dequeue packet
            cPacket *pkt = (cPacket*) buffer.pop();
            // send packet
            send(pkt, "out");
            // start new service
            serviceTime = pkt->getDuration();
            scheduleAt(simTime() + serviceTime, endServiceEvent);
        }
    // check buffer limit
    } else if (buffer.getLength() >= par("bufferSize").intValue()) {
        // Add 1 to the counter for each packet received
        packetReceived++;
        // drop the packet
        delete msg;
        this->bubble("packet dropped");
        packetDropVector.record(1);
    } else {
        // Add 1 to the counter for each packet received
        packetReceived++;
        // enqueue the packet
        buffer.insert(msg);
        bufferSizeVector.record(buffer.getLength());

        // If received more than (bufferSize/4) packets
        if (packetReceived > par("bufferSize").intValue() / 4) {
            // Set count to 0
            packetReceived = 0;

            // If buffer is half occupied
            if (buffer.getLength() >= par("bufferSize").intValue() / 2) {
                incrementServiceTime++;
                // Create FeedBackPacket to send a TraRx and then send to TraTx
                // and increase the serviceTime in 1
                FeedBackPacket *feedBackPkt = new FeedBackPacket("packet");
                feedBackPkt->setIncrementServiceTime(incrementServiceTime);
                feedBackPkt->setKind(2);
                bufferFeedPacket.insert(feedBackPkt);

            // If buffer is half free and incrementServiceTime > 1
            } else if (incrementServiceTime > 1 &&
                buffer.getLength() < par("bufferSize").intValue() / 2) {
                incrementServiceTime--;
                // Create FeedBackPacket to send a TraRx and then send to TraTx
                // and increase the serviceTime in 1
                FeedBackPacket *feedBackPkt = new FeedBackPacket("packet");
                feedBackPkt->setIncrementServiceTime(incrementServiceTime);
                feedBackPkt->setKind(2);
                bufferFeedPacket.insert(feedBackPkt);
            }
        }

        // if the server is idle
        if (!endServiceEvent->isScheduled()) {
            // start the service now
            scheduleAt(simTime() + 0, endServiceEvent);
        }
    }
}

#endif /* QUEUE */
