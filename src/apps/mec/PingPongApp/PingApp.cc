#include "inet/common/INETUtils.h"
#include "inet/common/packet/Packet.h"
#include "inet/networklayer/common/L3AddressResolver.h"
#include "inet/common/ModuleAccess.h"
#include "PingApp.h"

#include "nodes/mec/Federator/Messages/MEFmessages_m.h"
#include "inet/networklayer/common/L3Address.h"
#include "inet/transportlayer/common/L4PortTag_m.h"
#include "inet/networklayer/contract/ipv4/Ipv4Address.h"

Define_Module(PingApp);

PingApp::PingApp() {}

PingApp::~PingApp() {
    socket.close();
}

void PingApp::initialize(int stage) {
    DMecAppBaseDyn::initialize(stage);

    if (stage!=inet::INITSTAGE_APPLICATION_LAYER)
        return;

    localPort = par("localUePort");
    destPort = par("destPort");

    pingSending = 0;


    socket.setOutputGate(gate("socketOut"));
    socket.bind(localPort);

    EV_INFO << "PingApp:socketBind at: " << localPort << endl;

    cMessage* selfPing = new cMessage("sendPing");
    //scheduleAt(simTime() + 1, selfPing); // primo ping dopo 1s

    cMessage* requestApp = new cMessage("requestApp");
    scheduleAt(simTime() + 1, requestApp);

}

void PingApp::handleGenericMessage(cMessage* msg) {

    EV << "PingApp:handleGenericMessage" << endl;

    auto packet = dynamic_cast<Packet*>(msg);
    if (packet) {
        const char* name = packet->getName();
        auto src = packet->getTag<L3AddressInd>()->getSrcAddress();
        EV_INFO << "Received " << name << " from " << src << endl;

        if (strcmp(name, "appResponseVIMtoAPP") == 0) {
            auto data = packet->peekData<AppResponse>();
            destAddr = inet::L3AddressResolver().resolve(data->getAppAddress());
            destPort = data->getAppPort();

            EV << "PingApp:handleMessage - recived appAddress:port " << destAddr << ":" << destPort << endl;

            cMessage* selfPing = new cMessage("sendPing");
            scheduleAt(simTime(), selfPing);
        }

        delete packet;
    }

}

void PingApp::handleSelfMessage(cMessage* msg) {
    EV << "PingApp:handleSelfMessage " << msg->getName() << endl;
    if (strcmp(msg->getName(), "sendPing")==0) {
        sendPing();
        cMessage* selfPing = new cMessage("sendPing");
        scheduleAt(simTime()+1, selfPing);
    }else if (strcmp(msg->getName(), "requestApp")==0) {
        requestApp();
    }
}

void PingApp::sendPing() {
    pingSending++;
    EV << "Sending ping(" << pingSending << ") to " << destAddr << ":" << destPort << endl;
    auto packet = new Packet("Ping");
    const auto& payload = makeShared<ByteCountChunk>(B(4));
    packet->insertAtBack(payload);
    socket.sendTo(packet, destAddr, destPort);
}

void PingApp::requestApp(){
    std::string dest = getParentModule()->getFullPath();

    inet::Packet *packetResp = new inet::Packet("appRequestAPPtoVIM");

    auto request = inet::makeShared<AppRequest>();

    request->setAppName("PongApp");
    L3Address localAddr = L3AddressResolver().resolve(dest.c_str());

    request->setIpRequest(localAddr.str().c_str());
    request->setPortRequest(localPort);

    request->setChunkLength(inet::B(64));
    packetResp->insertAtBack(request);


    EV << "PingApp::requestApp - request: PongApp " << endl;

    //socket.sendTo(packetResp, inet::L3AddressResolver().resolve(par("meoAddr")), 2000);
    socket.sendTo(packetResp, localAddr, par("vimPort"));
}

void PingApp::finish() {
    DMecAppBaseDyn::finish();
}
