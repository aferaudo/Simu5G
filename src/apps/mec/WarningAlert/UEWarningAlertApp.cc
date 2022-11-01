//
//                  Simu5G
//
// Authors: Giovanni Nardini, Giovanni Stea, Antonio Virdis (University of Pisa)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "apps/mec/WarningAlert/UEWarningAlertApp.h"

#include "../DeviceApp/DeviceAppMessages/DeviceAppPacket_m.h"
#include "../DeviceApp/DeviceAppMessages/DeviceAppPacket_Types.h"

#include "apps/mec/WarningAlert/packets/WarningAlertPacket_m.h"
#include "apps/mec/WarningAlert/packets/WarningAlertPacket_Types.h"

#include "inet/common/TimeTag_m.h"
#include "inet/common/packet/chunk/BytesChunk.h"

#include "inet/networklayer/common/L3AddressTag_m.h"
#include "inet/transportlayer/common/L4PortTag_m.h"

#include <fstream>

using namespace inet;
using namespace std;

Define_Module(UEWarningAlertApp);

simsignal_t UEWarningAlertApp::logicTerminated_ = registerSignal("logicTerminated");

UEWarningAlertApp::UEWarningAlertApp(){
    selfStart_ = NULL;
    selfStop_ = NULL;
    amsHttpMessage = nullptr;
    amsHttpCompleteMessage = nullptr;
}

UEWarningAlertApp::~UEWarningAlertApp(){
    cancelAndDelete(selfStart_);
    cancelAndDelete(selfStop_);
    cancelAndDelete(selfMecAppStart_);
    cancelAndDelete(connectAmsMessage_);
    cancelAndDelete(subAmsMessage_);
}

void UEWarningAlertApp::initialize(int stage)
{
    EV << "UEWarningAlertApp::initialize - stage " << stage << endl;
    cSimpleModule::initialize(stage);
    // avoid multiple initializations
    if (stage!=inet::INITSTAGE_APPLICATION_LAYER)
        return;

    log = par("logger").boolValue();

    //retrieve parameters
    size_ = par("packetSize");
    period_ = par("period");
    localPort_ = par("localPort");
    deviceAppPort_ = par("deviceAppPort");
    sourceSimbolicAddress = (char*)getParentModule()->getFullName();
    deviceSimbolicAppAddress_ = (char*)par("deviceAppAddress").stringValue();
    deviceAppAddress_ = inet::L3AddressResolver().resolve(deviceSimbolicAppAddress_);

    // AMS parameters
    amsAddress = inet::L3AddressResolver().resolve(par("amsAddress").stringValue());
    amsPort = par("amsPort").intValue();

    // testing Core delays
    coreAddress = inet::L3AddressResolver().resolve(par("coreAddress").stringValue());
    coreTesting_ = par("coreTesting").boolValue();

    //binding socket
    socket.setOutputGate(gate("socketOut"));
    socket.bind(localPort_);
    amsSocket.setCallback(this);
    amsSocket.setOutputGate(gate("socketOut"));

    webHook ="/amsWebHook_" + std::to_string(getId());

    int tos = par("tos");
    if (tos != -1)
        socket.setTos(tos);

    //retrieving car cModule
    ue = this->getParentModule();

    //retrieving mobility module
    cModule *temp = getParentModule()->getSubmodule("mobility");
    if(temp != NULL){
        mobility = check_and_cast<inet::IMobility*>(temp);
    }
    else {
        EV << "UEWarningAlertApp::initialize - \tWARNING: Mobility module NOT FOUND!" << endl;
        throw cRuntimeError("UEWarningAlertApp::initialize - \tWARNING: Mobility module NOT FOUND!");
    }

    mecAppName = par("mecAppName").stringValue();

    //initializing the auto-scheduling messages
    selfStart_ = new cMessage("selfStart");
    selfStop_ = new cMessage("selfStop");
    selfMecAppStart_ = new cMessage("selfMecAppStart");
    subAmsMessage_ = new cMessage("subscribeAms");
    connectAmsMessage_ = new cMessage("connectAms");

    //starting UEWarningAlertApp
    simtime_t startTime = par("startTime");
    EV << "UEWarningAlertApp::initialize - starting sendStartMEWarningAlertApp() in " << startTime << " seconds " << endl;
    scheduleAt(simTime() + startTime, selfStart_);
    scheduleAt(simTime() + 0.5, connectAmsMessage_);

    //testing
    EV << "UEWarningAlertApp::initialize - sourceAddress: " << sourceSimbolicAddress << " [" << inet::L3AddressResolver().resolve(sourceSimbolicAddress).str()  <<"]"<< endl;
    EV << "UEWarningAlertApp::initialize - destAddress: " << deviceSimbolicAppAddress_ << " [" << deviceAppAddress_.str()  <<"]"<< endl;
    EV << "UEWarningAlertApp::initialize - binding to port: local:" << localPort_ << " , dest:" << deviceAppPort_ << endl;

    if(coreTesting_)
        allocatePingApp(coreAddress, false, true);

}

void UEWarningAlertApp::handleMessage(cMessage *msg)
{
    EV << "UEWarningAlertApp::handleMessage - " << msg->getFullName() << endl;
    // Sender Side
    if (msg->isSelfMessage())
    {
        EV << "UEWarningAlertApp::handleMessage - is self message: " << msg->getName() << endl;
        if(!strcmp(msg->getName(), "selfStart"))   sendStartMEWarningAlertApp();

        else if(!strcmp(msg->getName(), "selfStop"))    sendStopMEWarningAlertApp();

        else if(!strcmp(msg->getName(), "selfMecAppStart"))
        {
            sendMessageToMECApp();
            scheduleAt(simTime() + period_, selfMecAppStart_);
        }

        else if(!strcmp(msg->getName(), "connectAms")){
            connect(&amsSocket, amsAddress, amsPort);
        }
        else if(!strcmp(msg->getName(), "subscribeAms")){
            nlohmann::ordered_json subscriptionBody_;
            subscriptionBody_ = nlohmann::ordered_json();
            subscriptionBody_["_links"]["self"]["href"] = "";
            subscriptionBody_["callbackReference"] = deviceAppAddress_.str() + ":" + std::to_string(localPort_) + webHook;
            subscriptionBody_["requestTestNotification"] = false;
            subscriptionBody_["websockNotifConfig"]["websocketUri"] = "";
            subscriptionBody_["websockNotifConfig"]["requestWebsocketUri"] = false;
            subscriptionBody_["filterCriteria"]["appInstanceId"] = "";
            subscriptionBody_["filterCriteria"]["associateId"] = nlohmann::json::array();

            nlohmann::ordered_json val_;
            val_["type"] = "UE_IPv4_ADDRESS";
            val_["value"] = deviceAppAddress_.str();
            subscriptionBody_["filterCriteria"]["associateId"].push_back(val_);

            subscriptionBody_["filterCriteria"]["mobilityStatus"] = "INTERHOST_MOVEOUT_COMPLETED";
            subscriptionBody_["subscriptionType"] = "MobilityProcedureSubscription";
            EV << subscriptionBody_ << endl;

            std::string host = amsSocket.getRemoteAddress().str()+":"+std::to_string(amsSocket.getRemotePort());
            std::string uristring = "/example/amsi/v1/subscriptions/";
            Http::sendPostRequest(&amsSocket, subscriptionBody_.dump().c_str(), host.c_str(), uristring.c_str());
        }

        else    throw cRuntimeError("UEWarningAlertApp::handleMessage - \tWARNING: Unrecognized self message");

    }
    // Receiver Side
    else if(socket.belongsToSocket(msg)){
        inet::Packet* packet = check_and_cast<inet::Packet*>(msg);

        inet::L3Address ipAdd = packet->getTag<L3AddressInd>()->getSrcAddress();
        // int port = packet->getTag<L4PortInd>()->getSrcPort();

        /*
         * From Device app
         * device app usually runs in the UE (loopback), but it could also run in other places
         */
        if(ipAdd == deviceAppAddress_ || ipAdd == inet::L3Address("127.0.0.1")) // dev app
        {
            auto mePkt = packet->peekAtFront<DeviceAppPacket>();

            if (mePkt == 0)
                throw cRuntimeError("UEWarningAlertApp::handleMessage - \tFATAL! Error when casting to DeviceAppPacket");

            if( !strcmp(mePkt->getType(), ACK_START_MECAPP) )    handleAckStartMEWarningAlertApp(msg);

            else if(!strcmp(mePkt->getType(), ACK_STOP_MECAPP))  handleAckStopMEWarningAlertApp(msg);

            else
            {
                throw cRuntimeError("UEWarningAlertApp::handleMessage - \tFATAL! Error, DeviceAppPacket type %s not recognized", mePkt->getType());
            }
        }

        else
        {
            auto mePkt = packet->peekAtFront<WarningAppPacket>();
            if (mePkt == 0)
                throw cRuntimeError("UEWarningAlertApp::handleMessage - \tFATAL! Error when casting to WarningAppPacket");

            if(!strcmp(mePkt->getType(), WARNING_ALERT))      handleInfoMEWarningAlertApp(msg);
            else if(!strcmp(mePkt->getType(), START_NACK))
            {
                EV << "UEWarningAlertApp::handleMessage - MEC app did not started correctly, trying to start again" << endl;
            }
            else if(!strcmp(mePkt->getType(), START_ACK))
            {
                EV << "UEWarningAlertApp::handleMessage - MEC app started correctly" << endl;
                if(selfMecAppStart_->isScheduled())
                {
                    cancelEvent(selfMecAppStart_);
                }
            }
            else
            {
                throw cRuntimeError("UEWarningAlertApp::handleMessage - \tFATAL! Error, WarningAppPacket type %s not recognized", mePkt->getType());
            }
        }
        delete msg;
    }
    else if(amsSocket.belongsToSocket(msg))
    {
        amsSocket.processMessage(msg);
    }
}

void UEWarningAlertApp::finish()
{
    if(amsSocket.getState() == inet::TcpSocket::CONNECTED)
        amsSocket.close();
}
/*
 * -----------------------------------------------Sender Side------------------------------------------
 */
void UEWarningAlertApp::sendStartMEWarningAlertApp()
{
    inet::Packet* packet = new inet::Packet("WarningAlertPacketStart");
    auto start = inet::makeShared<DeviceAppStartPacket>();

    //instantiation requirements and info
    start->setType(START_MECAPP);
    start->setMecAppName(mecAppName.c_str());
    //start->setMecAppProvider("lte.apps.mec.warningAlert_rest.MEWarningAlertApp_rest_External");

    start->setChunkLength(inet::B(2+mecAppName.size()+1));
    start->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());

    packet->insertAtBack(start);

    socket.sendTo(packet, deviceAppAddress_, deviceAppPort_);

    if(log)
    {
        ofstream myfile;
        myfile.open ("example.txt", ios::app);
        if(myfile.is_open())
        {
            myfile <<"["<< NOW << "] UEWarningAlertApp - UE sent start message to the Device App \n";
            myfile.close();

        }
    }

    //rescheduling
    scheduleAt(simTime() + period_, selfStart_);
}
void UEWarningAlertApp::sendStopMEWarningAlertApp()
{
    EV << "UEWarningAlertApp::sendStopMEWarningAlertApp - Sending " << STOP_MEAPP <<" type WarningAlertPacket\n";

    inet::Packet* packet = new inet::Packet("DeviceAppStopPacket");
    auto stop = inet::makeShared<DeviceAppStopPacket>();

    //termination requirements and info
    stop->setType(STOP_MECAPP);

    stop->setChunkLength(inet::B(size_));
    stop->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());

    packet->insertAtBack(stop);
    socket.sendTo(packet, deviceAppAddress_, deviceAppPort_);

    if(log)
    {
        ofstream myfile;
        myfile.open ("example.txt", ios::app);
        if(myfile.is_open())
        {
            myfile <<"["<< NOW << "] UEWarningAlertApp - UE sent stop message to the Device App \n";
            myfile.close();
        }
    }

    //rescheduling
    if(selfStop_->isScheduled())
        cancelEvent(selfStop_);
    scheduleAt(simTime() + period_, selfStop_);
}

/*
 * ---------------------------------------------Receiver Side------------------------------------------
 */
void UEWarningAlertApp::handleAckStartMEWarningAlertApp(cMessage* msg)
{
    inet::Packet* packet = check_and_cast<inet::Packet*>(msg);
    auto pkt = packet->peekAtFront<DeviceAppStartAckPacket>();

    if(pkt->getResult() == true)
    {
        mecAppAddress_ = L3AddressResolver().resolve(pkt->getIpAddress());
        mecAppPort_ = pkt->getPort();
        EV << "UEWarningAlertApp::handleAckStartMEWarningAlertApp - Received " << pkt->getType() << " type WarningAlertPacket. mecApp isntance is at: "<< mecAppAddress_<< ":" << mecAppPort_ << endl;
        cancelEvent(selfStart_);
        //scheduling sendStopMEWarningAlertApp()
        if(!selfStop_->isScheduled()){
            simtime_t  stopTime = par("stopTime");
            scheduleAt(simTime() + stopTime, selfStop_);
            EV << "UEWarningAlertApp::handleAckStartMEWarningAlertApp - Starting sendStopMEWarningAlertApp() in " << stopTime << " seconds " << endl;
        }
        allocatePingApp(mecAppAddress_, false);
    }
    else
    {
        EV << "UEWarningAlertApp::handleAckStartMEWarningAlertApp - MEC application cannot be instantiated! Reason: " << pkt->getReason() << endl;
        return;
    }

    sendMessageToMECApp();
    scheduleAt(simTime() + period_, selfMecAppStart_);

}

void UEWarningAlertApp::sendMessageToMECApp(){

    // send star monitoring message to the MEC application

    inet::Packet* pkt = new inet::Packet("WarningAlertPacketStart");
    auto alert = inet::makeShared<WarningStartPacket>();
    alert->setType(START_WARNING);
    alert->setCenterPositionX(par("positionX").doubleValue());
    alert->setCenterPositionY(par("positionY").doubleValue());
    alert->setRadius(par("radius").doubleValue());
    alert->setChunkLength(inet::B(20));
    alert->addTagIfAbsent<inet::CreationTimeTag>()->setCreationTime(simTime());
    pkt->insertAtBack(alert);

    if(log)
    {
        ofstream myfile;
        myfile.open ("example.txt", ios::app);
        if(myfile.is_open())
        {
            myfile <<"["<< NOW << "] UEWarningAlertApp - UE sent start subscription message to the MEC application \n";
            myfile.close();
        }
    }

    socket.sendTo(pkt, mecAppAddress_ , mecAppPort_);
    EV << "UEWarningAlertApp::sendMessageToMECApp() - start Message sent to the MEC app" << endl;
}

void UEWarningAlertApp::handleInfoMEWarningAlertApp(cMessage* msg)
{
    inet::Packet* packet = check_and_cast<inet::Packet*>(msg);
    auto pkt = packet->peekAtFront<WarningAlertPacket>();

    EV << "UEWarningAlertApp::handleInfoMEWarningrAlertApp - Received " << pkt->getType() << " type WarningAlertPacket"<< endl;

    //updating runtime color of the car icon background
    if(pkt->getDanger())
    {
        if(log)
        {
            ofstream myfile;
            myfile.open ("example.txt", ios::app);
            if(myfile.is_open())
            {
                myfile <<"["<< NOW << "] UEWarningAlertApp - UE received danger alert TRUE from MEC application \n";
                myfile.close();
            }
        }

        EV << "UEWarningAlertApp::handleInfoMEWarningrAlertApp - Warning Alert Detected: DANGER!" << endl;
        ue->getDisplayString().setTagArg("i",1, "red");
    }
    else{
        if(log)
        {
            ofstream myfile;
            myfile.open ("example.txt", ios::app);
            if(myfile.is_open())
            {
                myfile <<"["<< NOW << "] UEWarningAlertApp - UE received danger alert FALSE from MEC application \n";
                myfile.close();
            }
        }

        EV << "UEWarningAlertApp::handleInfoMEWarningrAlertApp - Warning Alert Detected: NO DANGER!" << endl;
        ue->getDisplayString().setTagArg("i",1, "green");
    }
}
void UEWarningAlertApp::handleAckStopMEWarningAlertApp(cMessage* msg)
{

    inet::Packet* packet = check_and_cast<inet::Packet*>(msg);
    auto pkt = packet->peekAtFront<DeviceAppStopAckPacket>();

    EV << "UEWarningAlertApp::handleAckStopMEWarningAlertApp - Received " << pkt->getType() << " type WarningAlertPacket with result: "<< pkt->getResult() << endl;
    if(pkt->getResult() == false)
        EV << "Reason: "<< pkt->getReason() << endl;
    //updating runtime color of the car icon background
    ue->getDisplayString().setTagArg("i",1, "white");

    cancelEvent(selfStop_);

    EV << "Emitting... " << getParentModule()->getName() << endl;
    emit(logicTerminated_, getParentModule()->getName());

//    deallocatePingApp();
}

/*
 * ---------------------------------------------TCP Callback Implementation------------------------------------------
 */

void UEWarningAlertApp::connect(inet::TcpSocket* socket, const inet::L3Address& address, const int port)
{
    // we need a new connId if this is not the first connection
    socket->renewSocket();

    int timeToLive = par("timeToLive");
    if (timeToLive != -1)
        socket->setTimeToLive(timeToLive);

    int dscp = par("dscp");
    if (dscp != -1)
        socket->setDscp(dscp);

    int tos = par("tos");
    if (tos != -1)
        socket->setTos(tos);

    if (address.isUnspecified()) {
        EV_ERROR << "Connecting to " << address << " port=" << port << ": cannot resolve destination address\n";
    }
    else {
        EV_INFO << "Connecting to " << address << " port=" << port << endl;

        socket->connect(address, port);
    }
}

void UEWarningAlertApp::socketEstablished(TcpSocket * socket)
{
    EV << "UEWarningAlertApp::socketEstablished " << socket->getSocketId() << endl;

    scheduleAt(simTime() + 0.01, subAmsMessage_);
}

void UEWarningAlertApp::socketDataArrived(inet::TcpSocket *socket, inet::Packet *msg, bool)
{
    EV << "UEWarningAlertApp::socketDataArrived" << endl;


    std::vector<uint8_t> bytes =  msg->peekDataAsBytes()->getBytes();
    std::string packet(bytes.begin(), bytes.end());

    if(amsSocket.belongsToSocket(msg))
    {
        Http::parseReceivedMsg(amsSocket.getSocketId(), packet, completedMessageQueue, &bufferedData, &amsHttpMessage);
        while(completedMessageQueue.getLength() > 0)
        {
            amsHttpCompleteMessage = check_and_cast<HttpBaseMessage*>(completedMessageQueue.pop());
            if(amsHttpCompleteMessage->getType() == REQUEST){
                EV << "UEWarningAlertApp::socketDataArrived - Received notification - payload: " << " " << amsHttpCompleteMessage->getBody() << endl;
                HttpRequestMessage* amsNot = check_and_cast<HttpRequestMessage*>(amsHttpCompleteMessage);
                nlohmann::json jsonBody = nlohmann::json::parse(amsNot->getBody());
                if(!jsonBody.empty())
                {
                    nlohmann::json interfaces = nlohmann::json::array();
                    interfaces = jsonBody["targetAppInfo"]["commInterface"]["ipAddresses"];
                    mecAppAddress_ = L3AddressResolver().resolve(std::string(interfaces.at(0)["host"]).c_str()); // take first interface
                    mecAppPort_ = interfaces.at(0)["port"];
//                    deallocatePingApp();
//                    allocatePingApp(mecAppAddress_, true);
                    EV << "UEWarningAlertApp::received new mecapp address: " <<  mecAppAddress_.str() << ":" << mecAppPort_ << endl;
                }
            }else if(amsHttpCompleteMessage->getType() == RESPONSE){
                EV << "UEWarningAlertApp::socketDataArrived - Received response - payload: " << " " << amsHttpCompleteMessage->getBody() << endl;

                HttpResponseMessage* amsResponse = check_and_cast<HttpResponseMessage*>(amsHttpCompleteMessage);
                nlohmann::json jsonBody = nlohmann::json::parse(amsResponse->getBody());
                if(!jsonBody.empty()){
                    if(jsonBody.contains("callbackReference")){
                        std::stringstream stream;
                        stream << "sub" << jsonBody["subscriptionId"];
                        amsSubscriptionId = stream.str();
                        EV << "UEWarningAlertApp::socketDataArrived - subscription ID: " << amsSubscriptionId << endl;
                    }
                }

            }
        }
    }
    else{
        throw cRuntimeError("UEWarningAlertApp::socketDataArrived - Socket %d not recognized", socket->getSocketId());
    }

    delete msg;
}

void UEWarningAlertApp::socketPeerClosed(TcpSocket *socket_)
{
    EV << "UEWarningAlertApp::socketPeerClosed" << endl;
    socket_->close();
}

void UEWarningAlertApp::socketClosed(TcpSocket *socket)
{
    EV << "UEWarningAlertApp::socketClosed" << endl;
}


//  PING APP TEST
void UEWarningAlertApp::allocatePingApp(inet::L3Address mecAppAddress, bool pingMigrated, bool pingCore){
//    MECWarningAlertApp simu5g.apps.mec.WarningAlert.MECWarningAlertApp
    char* label;

    if(pingCore)
    {
        label = "core";
    }
    else if(pingMigrated){
        label = "migrated";
    }else{
        label = "initial";
    }


    char* meModuleName = "PingApp";
    cModuleType* moduleType = cModuleType::get("inet.applications.pingapp.PingApp");
    pingAppModule = moduleType->create(meModuleName, getParentModule());
    std::stringstream appName;
    appName << meModuleName << "["<< label << "]";
    pingAppModule->setName(appName.str().c_str());
    EV << "UEWarningAlertApp::handleInstantiation - meModuleName: " << appName.str() << endl;

    std::stringstream display;
    display << "p=" << 744 + (100 * int(pingMigrated)) << "," << 72 << ";i=block/control";
    pingAppModule->setDisplayString(display.str().c_str());

    pingAppModule->par("destAddr") = mecAppAddress.str();
    pingAppModule->finalizeParameters();

    cModule *at = getParentModule()->getSubmodule("at");
    cGate* newAtInGate = at->getOrCreateFirstUnconnectedGate("in", 0, false, true);
    cGate* newAtOutGate = at->getOrCreateFirstUnconnectedGate("out", 0, false, true);

    newAtOutGate->connectTo(pingAppModule->gate("socketIn"));
    pingAppModule->gate("socketOut")->connectTo(newAtInGate);

    pingAppModule->buildInside();
    pingAppModule->scheduleStart(simTime());
    pingAppModule->callInitialize();
}

void UEWarningAlertApp::deallocatePingApp(){
    pingAppModule->callFinish();
    pingAppModule->deleteModule();
}
