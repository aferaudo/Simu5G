

#include "apps/mec/MecApps/Dynamic/MecAppBaseDyn.h"

using namespace omnetpp;
using namespace inet;

MecAppBaseDyn::MecAppBaseDyn()
{
    mecHost = nullptr;
    serviceHttpMessage = nullptr;
    mp1HttpMessage = nullptr;
    vim = nullptr;
    vi = nullptr;
}

MecAppBaseDyn::~MecAppBaseDyn()
{

    if(serviceHttpMessage != nullptr)
    {
        delete serviceHttpMessage;
    }
    if(mp1HttpMessage != nullptr)
    {
        delete mp1HttpMessage;
    }


    if(processedServiceResponse->isScheduled())
    {
        cancelAndDelete(processedServiceResponse);
    }

    if(processedMp1Response->isScheduled())
    {
        cancelAndDelete(processedMp1Response);
    }

    if(processedAmsResponse->isScheduled())
    {
        cancelAndDelete(processedAmsResponse);
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

    serviceSocket_.setOutputGate(gate("socketOut"));
    mp1Socket_.setOutputGate(gate("socketOut"));
    amsSocket_.setOutputGate(gate("socketOut"));
    stateSocket_.setOutputGate(gate("socketOut"));
//    serverSocket_.setOutputGate(gate("socketOut"));

    serviceSocket_.setCallback(this);
    mp1Socket_.setCallback(this);
    amsSocket_.setCallback(this);
    stateSocket_.setCallback(this);
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
    //    vim = check_and_cast<VirtualisationInfrastructureManager*>(getParentModule()->getSubmodule("vim"));
//    mecHost = getParentModule();
//    mecPlatform = mecHost->getSubmodule("mecPlatform");
//    serviceRegistry = check_and_cast<ServiceRegistry *>(mecPlatform->getSubmodule("serviceRegistry"));

    processedServiceResponse = new cMessage("processedServiceResponse");
    processedMp1Response = new cMessage("processedMp1Response");
    processedAmsResponse = new cMessage("processedAmsResponse");
    processedStateResponse = new cMessage("processedStateResponse");
    processedStateResponse = new cMessage("processedInjectStateResponse");

}

void MecAppBaseDyn::socketDataArrived(inet::TcpSocket *socket, inet::Packet *msg, bool)
{
    EV << "MecAppBaseDyn::socketDataArrived" << endl;

    std::vector<uint8_t> bytes =  msg->peekDataAsBytes()->getBytes();
    std::string packet(bytes.begin(), bytes.end());
    EV << "data arrived" << endl;

    if(serviceSocket_.belongsToSocket(msg))
    {
        bool res =  Http::parseReceivedMsg(packet, &bufferedData, &serviceHttpMessage);
        if(res)
        {
            serviceHttpMessage->setSockId(serviceSocket_.getSocketId());
            if(vi == nullptr)
                throw cRuntimeError("MecAppBase::socketDataArrived - vi is null (service)!");
            double time = vi->calculateProcessingTime(mecAppId, 150);
            scheduleAt(simTime()+time, processedServiceResponse);
        }
    }
    else if (mp1Socket_.belongsToSocket(msg))
    {
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
        bool res =  Http::parseReceivedMsg(packet, &bufferedData, &amsHttpMessage);
        if(res)
        {
            amsHttpMessage->setSockId(amsSocket_.getSocketId());
            if(vi == nullptr)
                throw cRuntimeError("MecAppBase::socketDataArrived - vi is null! (ams)");
            double time = vi->calculateProcessingTime(mecAppId, 150);
            if(!processedAmsResponse->isScheduled()){
                scheduleAt(simTime()+time, processedAmsResponse);
            }
        }

    }
    else if (stateSocket_.belongsToSocket(msg))
    {
       EV << "it is a state message" << endl;
       stateMessage = check_and_cast<inet::Packet*>(msg);
       if(vi == nullptr)
           throw cRuntimeError("MecAppBase::socketDataArrived - vi is null (state)!");
       double time = vi->calculateProcessingTime(mecAppId, 150);
       scheduleAt(simTime()+time, processedStateResponse);

    }
    else {
        EV << "searching in socket map" << endl;
        inet::TcpSocket *socket = check_and_cast_nullable<inet::TcpSocket *>(socketMap.findSocketFor(msg));
        if (socket){
            injectStateMessage = check_and_cast<inet::Packet*>(msg);
            if(vim == nullptr)
                throw cRuntimeError("MecAppBase::socketDataArrived - vim is null (ams)!");
            double time = vim->calculateProcessingTime(mecAppId, 150);
            scheduleAt(simTime()+time, processedInjectStateResponse);
        }
        else {
            throw cRuntimeError("MecAppBase::socketDataArrived - Socket %d not recognized", socket->getSocketId());
        }
    }
//    else
//    {
//        throw cRuntimeError("MecAppBase::socketDataArrived - Socket %d not recognized", socket->getSocketId());
//    }
    delete msg;

}


