#ifndef NET
#define NET

#include <string.h>
#include <sstream>
#include <omnetpp.h>
#include <packet_m.h>

using namespace omnetpp;

class Net: public cSimpleModule {
private:
    int interfaces;
    struct NodeToSend {
        int indexGate;
        int lowestHopCount;
    };
    NodeToSend listNodes[100];
public:
    Net();
    virtual ~Net();
protected:
    virtual void initialize();
    virtual void finish();
    virtual void handleMessage(cMessage *msg);
};

Define_Module(Net);

#endif /* NET */

Net::Net() {
}

Net::~Net() {
}

void Net::initialize() {

    for (int i = 0; i < 100; i++) {
        listNodes[i].indexGate = 100;
        listNodes[i].lowestHopCount = 1000;
    }
    interfaces = this->getParentModule()->par("interfaces");

    // Send a HelloPacket to all interfaces available
    for (int i = 0; i < interfaces; i++) {
        cModule *node = this->getParentModule();
        cGate *gateConn = node->gate("toNod$i",i);
        if(gateConn->isConnected()){
            Packet *pktHello = new Packet("packet",this->getParentModule()->getIndex());
            pktHello->setSource(this->getParentModule()->getIndex());
            pktHello->setDestination(this->getParentModule()->getIndex());
            pktHello->setPacketHello(true);
            pktHello->setPacketEcho(false);
            send(pktHello, "toLnk$o", i);
        }
    }
}

void Net::finish() {
}

void Net::handleMessage(cMessage *msg) {

    // All msg (events) on net are packets
    Packet *pkt = (Packet *) msg;
    pkt->setHopCount(pkt->getHopCount() + 1);

    // If is a packet Hello check the hops to go to the source node
    if (pkt->getPacketHello()) {
        // If hops of the packet is < than the lowestHopCount
        if (pkt->getHopCount() < listNodes[pkt->getSource()].lowestHopCount) {
            // Change the lowestHopCount for the node
            listNodes[pkt->getSource()].lowestHopCount = pkt->getHopCount();
            // Change the indexGate to send for the node
            cGate *arrivalGate = pkt->getArrivalGate();
            listNodes[pkt->getSource()].indexGate = arrivalGate->getIndex();



            // Send packet Echo for where it came
            Packet *pktEcho = new Packet("packet",this->getParentModule()->getIndex());
            pktEcho->setSource(this->getParentModule()->getIndex());
            pktEcho->setDestination(pkt->getSource());
            pktEcho->setHopCount(0);
            pktEcho->setPacketHello(false);
            pktEcho->setPacketEcho(true);
            pktEcho->setTotalCount(pkt->getHopCount());

            send(pktEcho, "toLnk$o", arrivalGate->getIndex());
            //send packet Hello to the other gates
            for (int i = 0; i < interfaces; i++) {
                cModule *node = this->getParentModule();
                cGate *gateConn = node->gate("toNod$i",i);
                // To make string with name of gate and index
                std::ostringstream gate;
                gate << "toLnk$i[" << i << "]";
                std::string gateString = gate.str();

                // Send to gates that are available except where it came from
                if(gateConn->isConnected() && arrivalGate->getFullName() != gateString){
                    // Make copy of the packet to send
                    Packet *pktCopy = pkt->dup();

                    send(pktCopy, "toLnk$o", i);
                }
            }
        }
        delete(pkt);
    } // If it's a EchoPacket
    else if (pkt->getPacketEcho()) {
        // If it's the packet destination
        if (pkt->getDestination() ==  this->getParentModule()->getIndex()) {
            // If hops of the packet is < than the lowestHopCount
            if (pkt->getHopCount() < listNodes[pkt->getSource()].lowestHopCount) {
                // Change the lowestHopCount for the node
                listNodes[pkt->getSource()].lowestHopCount = pkt->getHopCount();

                // Change the indexGate to send for the node
                cGate *arrivalGate = pkt->getArrivalGate();
                listNodes[pkt->getSource()].indexGate = arrivalGate->getIndex();
            }
            // Delete packet Echo
            delete(pkt);
        } // Send to the appropriate gate to go to the node destination
        else {
            send(pkt, "toLnk$o", listNodes[pkt->getDestination()].indexGate);
        }
    } // else, is a data Packet
    else {
        // If this node is the final destination
        if (pkt->getDestination() == this->getParentModule()->getIndex()) {
            send(msg, "toApp$o");
        } // If not destination
        else {
            // If is a dataPacket send to the gate selected by the algorithm
            send(msg, "toLnk$o", listNodes[pkt->getDestination()].indexGate);
        }
    }
}
