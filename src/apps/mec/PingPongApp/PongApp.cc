#include "inet/common/INETUtils.h"
#include "inet/common/packet/Packet.h"
#include "inet/networklayer/common/L3AddressResolver.h"
#include "inet/common/ModuleAccess.h"
#include "PongApp.h"

#include "nodes/mec/Federator/Messages/MEFmessages_m.h"
#include "inet/networklayer/common/L3Address.h"
#include "inet/transportlayer/common/L4PortTag_m.h"
#include "inet/networklayer/contract/ipv4/Ipv4Address.h"

Define_Module(PongApp);

PongApp::PongApp() {

}

PongApp::~PongApp() {
    std::cout << ">> [PongApp::finish()] destructor called" << std::endl;
}

void PongApp::finish() {
    DMecAppBaseDyn::finish();

    if (socket.getState() == inet::UdpSocket::CONNECTED)
        socket.close();

    std::cout << ">> [PongApp::finish()] called" << std::endl;
}

void PongApp::initialize(int stage) {
    DMecAppBaseDyn::initialize(stage);

    if (stage!=inet::INITSTAGE_APPLICATION_LAYER)
        return;

    localPort = par("localUePort");
    destPort = par("destPort");

    socket.setOutputGate(gate("socketOut"));
    socket.bind(localPort);

    EV_INFO << "PongApp:socketBind at: " << localPort << endl;

}


void PongApp::handleGenericMessage(cMessage* msg) {
    auto packet = dynamic_cast<Packet*>(msg);
    if (packet) {
        const char* name = packet->getName();
        auto src = packet->getTag<L3AddressInd>()->getSrcAddress();
        auto srcPort = packet->getTag<L4PortInd>()->getSrcPort();
        EV_INFO << "Received " << name << " from " << src << endl;

        if (strcmp(name, "Ping") == 0) {
            EV << "PongApp:handleMessage - recived Ping and send Pong at: " << src << ":" << srcPort << endl;

            auto reply = new Packet("Pong");
            const auto& payload = makeShared<ByteCountChunk>(B(4));
            reply->insertAtBack(payload);
            socket.sendTo(reply, src, srcPort);
        }


        //delete packet;
    }
    delete msg;
}


void PongApp::handleSelfMessage(cMessage* msg) {

    EV << "PongApp:handleSelfMessage" << endl;

    delete msg;
}



