

#include "apps/mec/MecApps/Dynamic/MecAppBaseDyn.h"

using namespace omnetpp;
using namespace inet;

MecAppBaseDyn::MecAppBaseDyn()
{
    mecHost = nullptr;
    serviceHttpMessage = nullptr;
    bufferHttpMessageService = nullptr;
    bufferHttpMessageAms = nullptr;
    mp1HttpMessage = nullptr;
    vim = nullptr;
    vi = nullptr;
    stateSocket_ = nullptr;
}

MecAppBaseDyn::~MecAppBaseDyn()
{
//    if(serviceHttpMessage != nullptr)
//    {
//        delete serviceHttpMessage;
//    }
    if(bufferHttpMessageService != nullptr)
    {
        delete bufferHttpMessageService;
    }
    if(bufferHttpMessageAms != nullptr)
    {
        delete bufferHttpMessageAms;
    }
    if(mp1HttpMessage != nullptr)
    {
        delete mp1HttpMessage;
    }

//    cancelAndDelete(processedServiceResponse);
    cancelAndDelete(processedMp1Response);
//    cancelAndDelete(processedAmsResponse);
    cancelAndDelete(processedStateResponse);

    while(amsHttpMessages_.getLength())
    {
        delete amsHttpMessages_.pop();
    }
    while(serviceHttpMessages_.getLength())
    {
        delete serviceHttpMessages_.pop();
    }
}

void MecAppBaseDyn::initialize(int stage)
{

    if(stage != inet::INITSTAGE_APPLICATION_LAYER)
        return;

    registered = false;
    subscribed = false;

    const char *mp1Ip = par("mp1Address");
    mp1Address = L3AddressResolver().resolve(mp1Ip);
    mp1Port = par("mp1Port");

    stateSocket_ = new inet::TcpSocket();
    serviceSocket_.setOutputGate(gate("socketOut"));
    mp1Socket_.setOutputGate(gate("socketOut"));
    amsSocket_.setOutputGate(gate("socketOut"));
    stateSocket_->setOutputGate(gate("socketOut"));
//    serverSocket_.setOutputGate(gate("socketOut"));

    serviceSocket_.setCallback(this);
    mp1Socket_.setCallback(this);
    amsSocket_.setCallback(this);
    stateSocket_->setCallback(this);
//    serverSocket_.setCallback(this);

    mecAppId = par("mecAppId"); // FIXME mecAppId is the deviceAppId (it does not change anything, though)
    requiredRam = par("requiredRam").doubleValue();
    requiredDisk = par("requiredDisk").doubleValue();
    requiredCpu = par("requiredCpu").doubleValue();


//    vi = check_and_cast<VirtualisationInfrastructureApp*>(getParentModule()->getSubmodule("viApp"));
    if(getParentModule()->getSubmodule("app", 1) != nullptr)
    {
        // Generally viApp is the second deployed app
        vi = check_and_cast<VirtualisationInfrastructureApp*>(getParentModule()->getSubmodule("app", 1));
    }else
        vi = check_and_cast<VirtualisationInfrastructureApp*>(getParentModule()->getSubmodule("viApp"));

    processedServiceResponse = new cMessage("processedServiceResponse");
    processedMp1Response = new cMessage("processedMp1Response");
    processedAmsResponse = new cMessage("processedAmsResponse");
    processedStateResponse = new cMessage("processedStateResponse");

}

void MecAppBaseDyn::socketDataArrived(inet::TcpSocket *socket, inet::Packet *msg, bool)
{
    EV << "MecAppBaseDyn::socketDataArrived" << endl;

    if(serviceSocket_.belongsToSocket(msg))
    {
        std::vector<uint8_t> bytes =  msg->peekDataAsBytes()->getBytes();
        std::string packet(bytes.begin(), bytes.end());
        Http::parseReceivedMsg(serviceSocket_.getSocketId(), packet, serviceHttpMessages_, &bufferedDataService, &bufferHttpMessageService);

        if(serviceHttpMessages_.getLength() > 0)
        {
            if(vi == nullptr)
                throw cRuntimeError("MecAppBase::socketDataArrived - vi is null (service)!");
            double time = vi->calculateProcessingTime(mecAppId, 150);

            if(!processedServiceResponse->isScheduled())
                scheduleAt(simTime()+time, processedServiceResponse);
        }
    }
    else if (mp1Socket_.belongsToSocket(msg))
    {
        std::vector<uint8_t> bytes =  msg->peekDataAsBytes()->getBytes();
        std::string packet(bytes.begin(), bytes.end());
        bool res =  Http::parseReceivedMsg(packet, &bufferedData, &mp1HttpMessage);
        if(res)
        {
            mp1HttpMessage->setSockId(mp1Socket_.getSocketId());
            if(vi == nullptr)
                throw cRuntimeError("MecAppBase::socketDataArrived - vi is null! (mp1)");
            double time = vi->calculateProcessingTime(mecAppId, 150);
            scheduleAt(simTime()+time, processedMp1Response);
        }

    }
    else if (amsSocket_.belongsToSocket(msg))
    {
        if(amsHttpMessage != nullptr){
            amsHttpMessage = nullptr;
        }
        std::vector<uint8_t> bytes =  msg->peekDataAsBytes()->getBytes();
        std::string packet(bytes.begin(), bytes.end());
        Http::parseReceivedMsg(amsSocket_.getSocketId(), packet, amsHttpMessages_, &bufferedDataAms, &bufferHttpMessageAms);
        if(amsHttpMessages_.getLength() > 0)
        {
            if(vi == nullptr)
                throw cRuntimeError("MecAppBase::socketDataArrived - vi is null! (ams)");
            double time = vi->calculateProcessingTime(mecAppId, 150);
            if(!processedAmsResponse->isScheduled()){
                scheduleAt(simTime()+time, processedAmsResponse);
            }
        }

    }
    else if (serverSocket_.belongsToSocket(msg))
    {
       EV << "MECAppNaseDyn::it is a state message" << endl;
       stateMessage = check_and_cast<inet::Packet*>(msg)->dup();
       if(vi == nullptr)
           throw cRuntimeError("MecAppBase::socketDataArrived - vi is null (state)!");
       double time = vi->calculateProcessingTime(mecAppId, 150);
       if(!processedStateResponse->isScheduled())
           scheduleAt(simTime()+time, processedStateResponse);

    }
    else
    {
        throw cRuntimeError("MecAppBase::socketDataArrived - Socket %d not recognized", socket->getSocketId());
    }
    delete msg;

}

void MecAppBaseDyn::closeAllSockets() {
//    serviceSocket_.close();
//    mp1Socket_.close();
//    amsSocket_.close();
}

void MecAppBaseDyn::handleMessage(cMessage *msg)
{
    if(msg->isSelfMessage() && strcmp(msg->getName(), "processedAmsResponse") == 0)
    {
        amsHttpMessage = check_and_cast<HttpBaseMessage*>(amsHttpMessages_.pop());
        amsHttpMessage->setSockId(amsSocket_.getSocketId());
        handleAmsMessage();
        delete amsHttpMessage;
        if(amsHttpMessages_.getLength() > 0)
        {
            double time = vi->calculateProcessingTime(mecAppId, 150);
            if(!processedAmsResponse->isScheduled()){
                scheduleAt(simTime()+time, processedAmsResponse);
            }
        }
    }
    else if(msg->isSelfMessage() && strcmp(msg->getName(), "processedServiceResponse") == 0)
    {
        serviceHttpMessage = check_and_cast<HttpBaseMessage*>(serviceHttpMessages_.pop());
        serviceHttpMessage->setSockId(serviceSocket_.getSocketId());
        handleServiceMessage();
        //delete serviceHttpMessage;
        if(serviceHttpMessages_.getLength() > 0)
        {
            double time = vi->calculateProcessingTime(mecAppId, 150);
            if(!processedServiceResponse->isScheduled()){
                scheduleAt(simTime()+time, processedServiceResponse);
            }
        }
    }
    else
    {
        MecAppBase::handleMessage(msg);
    }
}

void MecAppBaseDyn::finish()
{
    EV << "MecAppBase::finish()" << endl;
    cancelAndDelete(processedServiceResponse); // discarding queue messages (these are typically delete replies)

    cancelAndDelete(processedAmsResponse); // discarding queue messages (these are typically delete replies)

    MecAppBase::finish();
}
