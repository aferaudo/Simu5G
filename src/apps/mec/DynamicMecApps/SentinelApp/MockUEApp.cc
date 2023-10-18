//
// 
// Date: 2023/10/10
// @author Angelo Feraudo

#include "MockUEApp.h"

#include "apps/mec/DeviceApp/DeviceAppMessages/DeviceAppPacket_m.h"
#include "apps/mec/DeviceApp/DeviceAppMessages/DeviceAppPacket_Types.h"
#include "inet/common/TimeTag_m.h"
#include "inet/common/packet/chunk/BytesChunk.h"
#include "inet/networklayer/common/L3AddressTag_m.h"
#include "inet/transportlayer/common/L4PortTag_m.h"

#include "inet/networklayer/common/L3Address.h"
#include "inet/networklayer/common/L3AddressResolver.h"


Define_Module(MockUEApp);

MockUEApp::MockUEApp()
{

}

MockUEApp::~MockUEApp()
{
    cancelAndDelete(selfStart_);
    cancelAndDelete(selfStop_);
    cancelAndDelete(selfMecAppStart_);
}

void MockUEApp::initialize(int stage)
{
     cSimpleModule::initialize(stage);
    // avoid multiple initializations
    if (stage!=inet::INITSTAGE_APPLICATION_LAYER)
        return;

    size_ = par("packetSize");
    period_ = par("period");
    localPort_ = par("localPort");
    deviceAppPort_ = par("deviceAppPort");
    sourceSimbolicAddress = (char*)getParentModule()->getFullName();
    deviceSimbolicAppAddress_ = (char*)par("deviceAppAddress").stringValue();
    deviceAppAddress_ = inet::L3AddressResolver().resolve(deviceSimbolicAppAddress_);

    //binding socket
    socket.setOutputGate(gate("socketOut"));
    socket.bind(localPort_);

    int tos = par("tos");
    if (tos != -1)
        socket.setTos(tos);

    //retrieving car cModule
    ue = this->getParentModule();

    mecAppName = par("mecAppName").stringValue();

    //initializing the auto-scheduling messages
    selfStart_ = new cMessage("selfStart");
    selfStop_ = new cMessage("selfStop");
    selfMecAppStart_ = new cMessage("selfMecAppStart");

    simtime_t startTime = par("startTime");

    //testing
    EV << "MockUEApp::initialize - sourceAddress: " << sourceSimbolicAddress << " [" << inet::L3AddressResolver().resolve(sourceSimbolicAddress).str()  <<"]"<< endl;
    EV << "MockUEApp::initialize - destAddress: " << deviceSimbolicAppAddress_ << " [" << deviceAppAddress_.str()  <<"]"<< endl;
    EV << "MockUEApp::initialize - binding to port: local:" << localPort_ << " , dest:" << deviceAppPort_ << endl;

    //starting MockUEApp
    scheduleAt(simTime() + startTime, selfStart_);


}


void MockUEApp::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage())
    {
        if (!strcmp(msg->getName(), "selfStart"))
        {
            sendStartMECApp();
        }
        else if (!strcmp(msg->getName(), "selfStop"))
        {
            sendStopMECApp();
        }
        else if(!strcmp(msg->getName(), "selfMecAppStart"))
        {
            sendMessageToMECApp();
            scheduleAt(simTime() + period_, selfMecAppStart_);
        }
    }
    else if(socket.belongsToSocket(msg))
    {
        inet::Packet* packet = check_and_cast<inet::Packet*>(msg);

        inet::L3Address ipAdd = packet->getTag<inet::L3AddressInd>()->getSrcAddress();
        // int port = packet->getTag<L4PortInd>()->getSrcPort();

        /*
         * From Device app
         * device app usually runs in the UE (loopback), but it could also run in other places
         */
        if(ipAdd == deviceAppAddress_ || ipAdd == inet::L3Address("127.0.0.1")) // dev app
        {
            auto deviceAppPacket = packet->peekAtFront<DeviceAppPacket>();

            if(!strcmp(deviceAppPacket->getType(), ACK_START_MECAPP))
            {
                EV << "MockUEApp::handleMessage - received ACK_START_MECAPP from device app" << endl;
                handleAckStartMECApp(msg);
            }
            else if(!strcmp(deviceAppPacket->getType(), ACK_STOP_MECAPP))
            {
                EV << "MockUEApp::handleMessage - received ACK_STOP_MECAPP from device app" << endl;
                handleAckStopMECApp(msg);
            }
            else if(!strcmp(deviceAppPacket->getType(), INFO_MECAPP))
            {
                EV << "MockUEApp::handleMessage - received INFO_MECAPP from device app" << endl;
                handleInfoMECApp(msg);
            }
            else
            {
                EV << "MockUEApp::handleMessage - received unknown message from device app" << endl;
                throw cRuntimeError("MockUEApp::handleMessage - \tFATAL! Error, DeviceAppPacket type %s not recognized", deviceAppPacket->getType());

            }
        }
    }
    else
    {
        // TODO - Define behavior for message received from the MEC application (if any)
    }
    
}

void MockUEApp::sendStartMECApp()
{
    inet::Packet* packet = new inet::Packet("MockAppStart");
    auto start = inet::makeShared<DeviceAppStartPacket>();

    //instantiation requirements and info
    start->setType(START_MECAPP);
    start->setMecAppName(mecAppName.c_str());

    start->setChunkLength(inet::B(2+mecAppName.size()+1));
    start->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());

    packet->insertAtBack(start);

    socket.sendTo(packet, deviceAppAddress_, deviceAppPort_);

    // rescheduling
    scheduleAt(simTime() + period_, selfStart_);
}

void MockUEApp::sendMessageToMECApp()
{
    // TODO - Generated method body
}

void MockUEApp::sendStopMECApp()
{
    EV << "MockUEApp::sendStopMEWarningAlertApp - Sending " << STOP_MECAPP <<" type MockUEApp\n";

    inet::Packet* packet = new inet::Packet("DeviceAppStopPacket");
    auto stop = inet::makeShared<DeviceAppStopPacket>();

    //termination requirements and info
    stop->setType(STOP_MECAPP);

    stop->setChunkLength(inet::B(size_));
    stop->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());

    packet->insertAtBack(stop);
    socket.sendTo(packet, deviceAppAddress_, deviceAppPort_);

    //rescheduling
    if(selfStop_->isScheduled())
        cancelEvent(selfStop_);
    scheduleAt(simTime() + period_, selfStop_);
    
}

void MockUEApp::handleAckStartMECApp(cMessage* msg)
{
    inet::Packet* packet = check_and_cast<inet::Packet*>(msg);
    auto pkt = packet->peekAtFront<DeviceAppStartAckPacket>();

    if(pkt->getResult() == true)
    {
        mecAppAddress_ = inet::L3AddressResolver().resolve(pkt->getIpAddress());
        mecAppPort_ = pkt->getPort();
        EV << "MockUEApp::handleAckStartMECApp - Received " << pkt->getType() << " type MockUEApp. mecApp isntance is at: "<< mecAppAddress_<< ":" << mecAppPort_ << endl;
        cancelEvent(selfStart_);
        //scheduling sendStopMEWarningAlertApp()
        if(!selfStop_->isScheduled()){
            simtime_t  stopTime = par("stopTime");
            scheduleAt(simTime() + stopTime, selfStop_);
            EV << "MockUEApp::handleAckStartMECApp - Starting sendStopMECApp() in " << stopTime << " seconds " << endl;
        }
    }
    else
    {
        EV << "MockUEApp::handleAckStartMECApp - MEC application cannot be instantiated! Reason: " << pkt->getReason() << endl;
        return;
    }
}

void MockUEApp::handleInfoMECApp(cMessage* msg)
{
    // TODO - Generated method body
}

void MockUEApp::handleAckStopMECApp(cMessage* msg)
{
    inet::Packet* packet = check_and_cast<inet::Packet*>(msg);
    auto pkt = packet->peekAtFront<DeviceAppStopAckPacket>();

    EV << "MockUEApp::handleAckStopMEWarningAlertApp - Received " << pkt->getType() << " type MockUEApp with result: "<< pkt->getResult() << endl;
    if(pkt->getResult() == false)
        EV << "Reason: "<< pkt->getReason() << endl;
    //updating runtime color of the car icon background

    cancelEvent(selfStop_);

}

void MockUEApp::handleMECAppInfo(cMessage *msg)
{
    // TODO - Generated method body
}

