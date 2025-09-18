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

void UEPingPongApp::finish()
{
    std::cout << ">> [UEPingPongApp::finish()] called" << std::endl;

    std::cout << "   Closing socket..." << std::endl;
    socket.close();
    std::cout << "   Socket closed." << std::endl;
}

UEPingPongApp::UEPingPongApp() {
    std::cout << ">> [UEPingPongApp::UEPingPongApp()] constructor" << std::endl;
}

UEPingPongApp::~UEPingPongApp() {
    std::cout << ">> [UEPingPongApp::~UEPingPongApp()] destructor called" << std::endl;


    std::cout << ">> [UEPingPongApp::~UEPingPongApp()] destructor end" << std::endl;
}



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


    cModule *ue = getParentModule();
    lastMasterId = ue->par("nrMasterId").intValue();

    EV << ">> gNB cambiato! Da " << lastMasterId << " a " << lastMasterId << endl;
    const char *newColor = (lastMasterId == 1) ? "blue" : ((lastMasterId == 2) ? "green" : "red");

    cDisplayString& disp = getParentModule()->getDisplayString();
    disp.setTagArg("i", 1, newColor);

    if (auto rect = dynamic_cast<cRectangleFigure*>(getParentModule()
                               ->getCanvas()->getFigure("linkLayer"))) {
        rect->setFillColor(newColor);
        rect->setLineColor(newColor);
    }



    scheduleAt(simTime()+startTime, new cMessage("sendStart"));
    scheduleAt(simTime() + 0.1, new cMessage("checkHandover"));

}



void UEPingPongApp::handleMessage(cMessage *msg)
{
    //EV << "UEPingPongApp::handleMessage" << endl;
    if (msg->isSelfMessage()){
        if (strcmp(msg->getName(), "sendStart") == 0) {
            EV << "UEPingPongApp::handleMessage - sendStart" << endl;
            const char *destAddrStr = par("deviceAppAddress");
            destAddress = L3AddressResolver().resolve(destAddrStr);

            sendStart();
            delete msg;
        }else if (strcmp(msg->getName(), "checkHandover") == 0) {
            cModule *ue = getParentModule();
            int currentMasterId = ue->par("nrMasterId").intValue();
            if (currentMasterId != lastMasterId) {
                EV << ">> gNB cambiato! Da " << lastMasterId << " a " << currentMasterId << endl;
                lastMasterId = currentMasterId;

                const char *newColor = (lastMasterId == 1) ? "blue" : ((lastMasterId == 2) ? "green" : "red");

                cDisplayString& disp = getParentModule()->getDisplayString();
                disp.setTagArg("i", 1, newColor);

                if (auto rect = dynamic_cast<cRectangleFigure*>(getParentModule()
                                           ->getCanvas()->getFigure("linkLayer"))) {
                    rect->setFillColor(newColor);
                    rect->setLineColor(newColor);
                }
            }
            scheduleAt(simTime() + 0.1, new cMessage("checkHandover"));
            delete msg;
        }else{
            std::cout << "[UEPingPongApp::handleMessage()] other message - " << msg->getName() << endl;
        }
    }
    else {
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
