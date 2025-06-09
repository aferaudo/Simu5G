/*
 * UEPingPongApp.cc
 *
 *  Created on: May 30, 2025
 *      Author: simulator
 */


#include "UEPingPongApp.h"
#include "inet/networklayer/common/L3AddressResolver.h"

#include "../DeviceApp/DeviceAppMessages/DeviceAppPacket_m.h"
#include "../DeviceApp/DeviceAppMessages/DeviceAppPacket_Types.h"

#include "inet/common/TimeTag_m.h"
#include "inet/common/packet/chunk/BytesChunk.h"

#include "inet/networklayer/common/L3AddressTag_m.h"
#include "inet/transportlayer/common/L4PortTag_m.h"

#include "UEPingPongApp.h"
#include "inet/networklayer/common/L3AddressResolver.h"


Define_Module(UEPingPongApp);

void UEPingPongApp::initialize(int stage)
{
    EV << "UEPingPongApp::initialize - stage " << stage << endl;
    cSimpleModule::initialize(stage);
    if (stage!=inet::INITSTAGE_APPLICATION_LAYER)
            return;

    localPort = par("localPort");
    destPort = par("deviceAppPort");

    startTime = par("startTime");

    socket.setOutputGate(gate("socketOut"));
    socket.bind(localPort);

    mecAppName = par("mecAppName").stringValue();

    scheduleAt(startTime, new cMessage("sendStart"));
}

void UEPingPongApp::handleMessage(cMessage *msg)
{
    EV << "UEPingPongApp::handleMessage" << endl;
    if (strcmp(msg->getName(), "sendStart") == 0) {
        EV << "UEPingPongApp::handleMessage - sendStart" << endl;
        const char *destAddrStr = par("deviceAppAddress");
        destAddress = L3AddressResolver().resolve(destAddrStr);

        sendStart();
        delete msg;
    } else {
        socket.processMessage(msg);
    }
}

void UEPingPongApp::sendStart()
{
    EV << "UEPingPongApp::sendStart" << endl;

    inet::Packet* packet = new inet::Packet("StartMECApp");
    auto start = makeShared<DeviceAppStartPacket>();
    start->setType(START_MECAPP);
    start->setMecAppName(mecAppName.c_str());

    start->setChunkLength(inet::B(2+mecAppName.size()+1));
    start->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());

    packet->insertAtBack(start);

    EV << "UEPingPongApp::sendStart with parameter: destAddress: " << destAddress << " - destPort: " << destPort<< endl;
    socket.sendTo(packet, destAddress, destPort);
}
