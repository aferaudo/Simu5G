

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

}

void MecAppBaseDyn::initialize(int stage)
{

    if(stage != inet::INITSTAGE_APPLICATION_LAYER)
        return;

    const char *mp1Ip = par("mp1Address");
    mp1Address = L3AddressResolver().resolve(mp1Ip);
    mp1Port = par("mp1Port");

    serviceSocket_.setOutputGate(gate("socketOut"));
    mp1Socket_.setOutputGate(gate("socketOut"));

    serviceSocket_.setCallback(this);
    mp1Socket_.setCallback(this);

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

}

void MecAppBaseDyn::socketDataArrived(inet::TcpSocket *socket, inet::Packet *msg, bool)
{
    EV << "MecAppBaseDyn::socketDataArrived" << endl;

    std::vector<uint8_t> bytes =  msg->peekDataAsBytes()->getBytes();
    std::string packet(bytes.begin(), bytes.end());
//    EV << packet << endl;

    if(serviceSocket_.belongsToSocket(msg))
    {
        bool res =  Http::parseReceivedMsg(packet, &bufferedData, &serviceHttpMessage);
        if(res)
        {
            serviceHttpMessage->setSockId(serviceSocket_.getSocketId());
            if(vi == nullptr)
                throw cRuntimeError("MecAppBase::socketDataArrived - vim is null!");
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
                throw cRuntimeError("MecAppBase::socketDataArrived - vim is null!");
            double time = vi->calculateProcessingTime(mecAppId, 150);
            scheduleAt(simTime()+time, processedMp1Response);
        }

    }
    else
    {
        throw cRuntimeError("MecAppBase::socketDataArrived - Socket %d not recognized", socket->getSocketId());
    }
    delete msg;

}
