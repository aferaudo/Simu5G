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

#include "apps/mec/DynamicMecApps/MecAppBase/DMecAppBase.h"
#include "nodes/mec/utils/httpUtils/httpUtils.h"
#include "nodes/mec/utils/MecCommon.h"
#include "common/utils/utils.h"

using namespace omnetpp;
using namespace inet;


DMecAppBase::DMecAppBase()
{
    mecHost = nullptr;
    mecPlatform = nullptr;;
    serviceRegistry = nullptr;
    sendTimer = nullptr;
    serviceHttpMessage = nullptr;
    mp1HttpMessage = nullptr;
    vim = nullptr;
}

DMecAppBase::~DMecAppBase()
{
    if(sendTimer != nullptr)
    {
        if(sendTimer->isSelfMessage())
            cancelAndDelete(sendTimer);
        else
            delete sendTimer;
    }

    if(serviceHttpMessage != nullptr)
    {
        delete serviceHttpMessage;
    }

    if(mp1HttpMessage != nullptr)
    {
        delete mp1HttpMessage;
    }


    // TODO: Segmentation fault if these line are not commented. Why?
//    if(processedServiceResponse->isScheduled())
//    {
//        cancelAndDelete(processedServiceResponse);
//    }

//    if(processedMp1Response->isScheduled())
//    {
//        cancelAndDelete(processedMp1Response);
//    }

//    if(processedAmsResponse->isScheduled())
//    {
//        cancelAndDelete(processedAmsResponse);
//    }
}

void DMecAppBase::initialize(int stage)
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

    vim = check_and_cast<VirtualisationInfrastructureManager*>(getParentModule()->getSubmodule("vim"));
    mecHost = getParentModule();
    mecPlatform = mecHost->getSubmodule("mecPlatform");

    serviceRegistry = check_and_cast<ServiceRegistry *>(mecPlatform->getSubmodule("serviceRegistry"));

    processedServiceResponse = new cMessage("processedServiceResponse");
    processedMp1Response = new cMessage("processedMp1Response");

}

void DMecAppBase::connect(inet::TcpSocket* socket, const inet::L3Address& address, const int port)
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

void DMecAppBase::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage())
    {
        EV << "DMecAppBase::handleMessage" << endl;
        if(strcmp(msg->getName(), "processedServiceResponse") == 0)
        {
            handleServiceMessage();
            if(serviceHttpMessage != nullptr)
            {
                delete serviceHttpMessage;
                serviceHttpMessage = nullptr;
            }
        }
        else if (strcmp(msg->getName(), "processedMp1Response") == 0)
        {
            handleMp1Message();
            if(mp1HttpMessage != nullptr)
            {
                delete mp1HttpMessage;
                mp1HttpMessage = nullptr;
            }
        }
        else if (strcmp(msg->getName(), "processedAmsResponse") == 0)
        {
            handleAmsMessage();
//            if(amsHttpMessage != nullptr)
//            {
//                delete amsHttpMessage;
//                amsHttpMessage = nullptr;
//            }
        }
        else if (strcmp(msg->getName(), "processedStateResponse") == 0)
        {
            handleStateMessage();

            if(stateMessage != nullptr)
            {
                delete stateMessage;
                stateMessage = nullptr;
            }
        }
        else
        {
            handleSelfMessage(msg);
        }
    }
    else
    {
        // from service or from ue app?
        if(serviceSocket_.belongsToSocket(msg))
        {
            serviceSocket_.processMessage(msg);
        }
        else if(mp1Socket_.belongsToSocket(msg))
        {
            mp1Socket_.processMessage(msg);
        }
        else if(amsSocket_.belongsToSocket(msg))
        {
            amsSocket_.processMessage(msg);
        }
        else if(stateSocket_->belongsToSocket(msg))
        {
            stateSocket_->processMessage(msg);
        }
        else if(serverSocket_.belongsToSocket(msg))
        {
            serverSocket_.processMessage(msg);
        }

    }
}

void DMecAppBase::socketEstablished(TcpSocket * socket)
{
    EV << "DMecAppBase::socketEstablished " << socket->getSocketId() << endl;
    established(socket->getSocketId());
}

void DMecAppBase::socketDataArrived(inet::TcpSocket *socket, inet::Packet *msg, bool)
{
    EV << "DMecAppBase::socketDataArrived" << endl;


//  In progress. Trying to handle an app message coming from different tcp segments in a better way
//    bool res = msg->hasAtFront<HttpResponseMessage>();
//    auto pkc = msg->peekAtFront<HttpResponseMessage>(b(-1), Chunk::PF_ALLOW_EMPTY | Chunk::PF_ALLOW_SERIALIZATION | Chunk::PF_ALLOW_INCOMPLETE);
//    if(pkc!=nullptr && pkc->isComplete() &&  !pkc->isEmpty()){
//
//    }
//    else
//    {
//        EV << "Null"<< endl;
//    }


//
//    EV << "DMecAppBase::socketDataArrived - payload: " << endl;
//
    std::vector<uint8_t> bytes =  msg->peekDataAsBytes()->getBytes();
    std::string packet(bytes.begin(), bytes.end());
    EV << "data arrived" << endl;

    if(serviceSocket_.belongsToSocket(msg))
    {
        bool res =  Http::parseReceivedMsg(packet, &bufferedData, &serviceHttpMessage);
        if(res)
        {
            serviceHttpMessage->setSockId(serviceSocket_.getSocketId());
            if(vim == nullptr)
                throw cRuntimeError("DMecAppBase::socketDataArrived - vim is null (service)!");
            double time = vim->calculateProcessingTime(mecAppId, 150);
            scheduleAt(simTime()+time, processedServiceResponse);
        }
    }
    else if (mp1Socket_.belongsToSocket(msg))
    {
        bool res =  Http::parseReceivedMsg(packet, &bufferedData, &mp1HttpMessage);
        if(res)
        {
            mp1HttpMessage->setSockId(mp1Socket_.getSocketId());
            if(vim == nullptr)
                throw cRuntimeError("DMecAppBase::socketDataArrived - vim is null (mp1)!");
            double time = vim->calculateProcessingTime(mecAppId, 150);
            scheduleAt(simTime()+time, processedMp1Response);
        }

    }
    else if (amsSocket_.belongsToSocket(msg))
    {
        std::cout << "AMS base " << endl;
        bool res =  Http::parseReceivedMsg(packet, &bufferedData, &amsHttpMessage);
        if(res)
        {
            amsHttpMessage->setSockId(amsSocket_.getSocketId());
            if(vim == nullptr)
                throw cRuntimeError("DMecAppBase::socketDataArrived - vim is null (ams)!");
            double time = vim->calculateProcessingTime(mecAppId, 150);
            scheduleAt(simTime()+time, processedAmsResponse);
        }

    }
    else if (stateSocket_->belongsToSocket(msg))
    {
        EV << "it is a state message" << endl;
        stateMessage = check_and_cast<inet::Packet*>(msg);
        if(vim == nullptr)
            throw cRuntimeError("DMecAppBase::socketDataArrived - vim is null (ams)!");
        double time = vim->calculateProcessingTime(mecAppId, 150);
        scheduleAt(simTime()+time, processedStateResponse);

    }
    else
    {
        throw cRuntimeError("DMecAppBase::socketDataArrived - Socket %d not recognized", socket->getSocketId());
    }
    delete msg;

}

void DMecAppBase::socketAvailable(inet::TcpSocket *socket, inet::TcpAvailableInfo *availableInfo)
{
    EV << "DMecAppBase::socketAvailable - accepting " << availableInfo->getNewSocketId() << endl;

    stateSocket_ = new TcpSocket(availableInfo);
    stateSocket_->setOutputGate(gate("socketOut"));
    stateSocket_->setCallback(this);

    EV << "DMecAppBase::stateSocket created with id " << stateSocket_->getSocketId() << endl;


    serverSocket_.accept(availableInfo->getNewSocketId());
    EV << "DMecAppBase::only one open socket is supported: stop listening!" << endl;
}

void DMecAppBase::socketPeerClosed(TcpSocket *socket_)
{
    EV << "DMecAppBase::socketPeerClosed" << endl;
    std::cout<<"Closing peer socket MECApp Base" << endl;
//    socket_->close();
    socket_->setState(inet::TcpSocket::PEER_CLOSED);
}

void DMecAppBase::socketClosed(TcpSocket *socket)
{
    EV_INFO << "DMecAppBase::socketClosed" << endl;
}

void DMecAppBase::socketFailure(TcpSocket *sock, int code)
{
    // subclasses may override this function, and add code try to reconnect after a delay.
}

void DMecAppBase::finish()
{
    EV << "DMecAppBase::finish()" << endl;
    if(serviceSocket_.getState() == inet::TcpSocket::CONNECTED){
        serviceSocket_.close();
//        serviceSocket_.setState(inet::TcpSocket::CLOSED);
    }
    if(mp1Socket_.getState() == inet::TcpSocket::CONNECTED){
        mp1Socket_.close();
//        mp1Socket_.setState(inet::TcpSocket::CLOSED);
    }
    if(amsSocket_.getState() == inet::TcpSocket::CONNECTED){
        amsSocket_.close();
//        amsSocket_.setState(inet::TcpSocket::CLOSED);
    }
    if(stateSocket_->getState() == inet::TcpSocket::CONNECTED){
        stateSocket_->close();
//        stateSocket_->setState(inet::TcpSocket::CLOSED);
    }

}



