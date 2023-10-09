
#include "ExtMecAppBase.h"
#include "apps/mec/MecApps/packets/ProcessingTimeMessage_m.h"

using namespace omnetpp;

ExtMecAppBase::ExtMecAppBase()
{
   processMessage_ = nullptr;
   terminationMessage_ = nullptr;
   connectService_ = nullptr;
   mp1Socket_ = nullptr;

}

ExtMecAppBase::~ExtMecAppBase()
{
    std::cout << "destructor called!" << endl;
    for(auto &sock : sockets_.getMap())
    {

        inet::TcpSocket* tcpSock = (inet::TcpSocket*)sock.second;
        removeSocket(tcpSock);
    }
    sockets_.deleteSockets();
    std::cout << "socket ok" << endl;

    if(processMessage_ != nullptr)
        cancelAndDelete(processMessage_);
    std::cout << "processMessage ok" << endl;

    if(terminationMessage_ != nullptr)
        cancelAndDelete(terminationMessage_);
    std::cout << "terminationMessage ok" << endl;

    if(connectService_ != nullptr)
        cancelAndDelete(connectService_);
    std::cout << "connectService ok" << endl;


//    while(!packetQueue_.isEmpty())
//        delete packetQueue_.pop();
//    std::cout << "packetQueue ok" << endl;

}

void ExtMecAppBase::initialize(int stage)
{
    if(stage != inet::INITSTAGE_APPLICATION_LAYER)
        return;

    const char *mp1Ip = par("mp1Address");
    mp1Address = inet::L3AddressResolver().resolve(mp1Ip);
    mp1Port = par("mp1Port");

    mecAppId = par("mecAppId"); // FIXME mecAppId is the deviceAppId (it does not change anything, though)
//    mecAppIndex_ = par("mecAppIndex"); // not needed
    requiredRam = par("requiredRam").doubleValue();
    requiredDisk = par("requiredDisk").doubleValue();
    requiredCpu = par("requiredCpu").doubleValue();

    localAddress = inet::L3AddressResolver().resolve(getParentModule()->getFullPath().c_str());

    processMessage_ = new cMessage("processedMessage");

    terminationMessage_ = new cMessage("terminationMessage");

    connectService_ = new cMessage("connectService");

    responseCounter_ = 0;
}

void ExtMecAppBase::handleMessage(omnetpp::cMessage *msg)
{
    EV << "Processing message " << msg->getName() << endl;
    if(msg->isSelfMessage())
    {
        if(strcmp(msg->getName(), "processedMessage") == 0)
        {
           handleProcessedMessage((cMessage*)packetQueue_.pop());

           if(!packetQueue_.isEmpty())
           {
               double processingTime = 0;
               EV <<"ExtMecAppBase::scheduleNextMsg() - next msg is processed in " << processingTime << "s" << endl;
               scheduleAt(simTime() + processingTime, processMessage_);
           }
           else
           {
               EV << "MecAppBase::handleMessage - no more messages are present in the queue" << endl;
           }
        }
        else if(strcmp(msg->getName(), "processedHttpMsg") == 0)
        {
           EV << "MecAppBase::handleMessage(): processedHttpMsg " << endl;
           ProcessingTimeMessage * procMsg = check_and_cast<ProcessingTimeMessage*>(msg);
           int connId = procMsg->getSocketId();
           inet::TcpSocket* sock = (inet::TcpSocket*)sockets_.getSocketById(connId);
           if(sock != nullptr)
           {
               HttpMessageStatus* msgStatus = (HttpMessageStatus *)sock->getUserData();
               handleHttpMessage(connId);



               delete msgStatus->httpMessageQueue.pop();
               if(!msgStatus->httpMessageQueue.isEmpty())
               {
                   EV << "ExtMecAppBase::handleMessage(): processedHttpMsg - the httpMessageQueue is not empty, schedule next HTTP message" << endl;
                   double time = 0;
                   scheduleAt(simTime()+time, msg);
               }
           }
        }
        else if(strcmp(msg->getName(), "terminationMessage") == 0)
        {
            EV << "ExtMecAppBase::Received termination message " << getFullName() << " - " << getParentModule()->getFullName() <<", waiting for: " << responseCounter_ << " replies" << endl;
            if(responseCounter_ > 0)
            {
                scheduleAt(simTime()+0.01, terminationMessage_);
                return;
            }
            else
            {
                cGate* gate = this->gate("viAppGate$o");
                cMessage* t = new cMessage("endTerminationProcedure");
                send(t, gate->getName());
                callFinish();
                deleteModule();
            }
        }
        else
        {
            handleSelfMessage(msg);
        }
    }
    else if(strcmp(msg->getName(), "startTerminationProcedure") == 0)
    {
        // Extended system
        EV << "ExtMecAppBase::start termination procedure" << endl;
        handleTermination();
        delete msg;
    }
    else
    {
        if(!processMessage_->isScheduled() && packetQueue_.isEmpty())
        {
            packetQueue_.insert(msg);
            // TODO do we have any processing time?
            double processingTime = 0;
            scheduleAt(simTime() + processingTime, processMessage_);
        }
        else if (processMessage_->isScheduled() && !packetQueue_.isEmpty())
        {
            packetQueue_.insert(msg);
        }
        else
        {
            throw cRuntimeError("ExtMecAppBase::handleMessage - This situation is not possible");
        }
    }
}

void ExtMecAppBase::finish()
{
}

void ExtMecAppBase::socketDataArrived(inet::TcpSocket *socket,
        inet::Packet *msg, bool urgent)
{
    EV << "ExtMecAppBase::socketDataArrived" << endl;
    if(socket->getUserData() != nullptr)
    {

        std::vector<uint8_t> bytes =  msg->peekDataAsBytes()->getBytes();
        std::string packet(bytes.begin(), bytes.end());

        // Getting user data
        HttpMessageStatus* msgStatus = (HttpMessageStatus*) socket->getUserData();

        bool res =  Http::parseReceivedMsg(socket->getSocketId(),  packet, msgStatus->httpMessageQueue , &(msgStatus->bufferedData),  &(msgStatus->currentMessage));

        if(res)
        {
            double time = 0; // In our system we don't care about computation time.
            if(!msgStatus->processMsgTimer->isScheduled())
                scheduleAt(simTime()+time, msgStatus->processMsgTimer);
        }
    }
    else
    {
        handleReceivedMessage(socket->getSocketId(), msg);
    }

    delete msg;

}

void ExtMecAppBase::socketAvailable(inet::TcpSocket *socket, inet::TcpAvailableInfo *availableInfo)
{
    EV << "ExtMecAppBase::socket in listening mode has been added" << endl;
    inet::TcpSocket *newSocket = new inet::TcpSocket(availableInfo);
    newSocket->setOutputGate(gate("socketOut"));
    newSocket->setCallback(this);
    sockets_.addSocket(newSocket);

    socket->accept(availableInfo->getNewSocketId());

}

void ExtMecAppBase::socketEstablished(inet::TcpSocket *socket)
{
    established(socket->getSocketId());
}

void ExtMecAppBase::socketPeerClosed(inet::TcpSocket *socket)
{
    EV << "ExtMecAppBase::Peer closed the socket " << socket->getRemoteAddress() << endl;
    socket->close();

}

void ExtMecAppBase::socketClosed(inet::TcpSocket *socket)
{
    EV_INFO << "ExtMecAppBase:: socket closed " <<  endl;
    EV << "ExtMecAppBase::Removing socket after closing" << endl;

}

void ExtMecAppBase::socketFailure(inet::TcpSocket *socket, int code)
{
}

void ExtMecAppBase::connect(inet::TcpSocket *socket,
        const inet::L3Address &address, const int port)
{
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

void ExtMecAppBase::handleProcessedMessage(omnetpp::cMessage *msg)
{
    if(msg->isSelfMessage())
    {
        handleSelfMessage(msg);
    }
    else
    {
        inet::TcpSocket* sock = (inet::TcpSocket*)sockets_.findSocketFor(msg);
        if(sock != nullptr)
        {
            EV << "ExtMecAppBase::handleProcessedMessage(): message for socket with ID: " << sock->getSocketId() << endl;
            sock->processMessage(msg);
        }
        else
        {
            EV << "Message has been deleted" << endl;
            delete msg;
        }
    }
}

inet::TcpSocket* ExtMecAppBase::addNewSocket()
{
    inet::TcpSocket* newSocket = new inet::TcpSocket();
    newSocket->setOutputGate(gate("socketOut"));
    newSocket->setCallback(this);

    // this creates sockets that transport http content
    HttpMessageStatus* msg = new HttpMessageStatus;
    msg->processMsgTimer = new ProcessingTimeMessage("processedHttpMsg");
    msg->processMsgTimer->setSocketId(newSocket->getSocketId());
    newSocket->setUserData(msg);


    sockets_.addSocket(newSocket);
    EV << "ExtMecAppBase::addNewSocket(): added socket with ID: " << newSocket->getSocketId() << endl;
    return newSocket;
}

inet::TcpSocket* ExtMecAppBase::addNewSocketNoHttp()
{
    inet::TcpSocket* newSocket = new inet::TcpSocket();
    newSocket->setOutputGate(gate("socketOut"));
    newSocket->setCallback(this);
    sockets_.addSocket(newSocket);
    EV << "ExtMecAppBase::addNewSocketNoHttp(): added socket for no http messages with ID: " << newSocket->getSocketId() << endl;
    return newSocket;
}

void ExtMecAppBase::removeSocket(inet::TcpSocket* tcpSock)
{
    if(tcpSock->getUserData() != nullptr)
    {
        HttpMessageStatus* msgStatus = (HttpMessageStatus *) tcpSock->getUserData();
        EV << "ExtMecAppBase::removing socket" << endl;
        while(!msgStatus->httpMessageQueue.isEmpty())
        {
            delete msgStatus->httpMessageQueue.pop();
        }
        msgStatus->httpMessageQueue.clear();
        if(msgStatus->currentMessage != nullptr )
            delete msgStatus->currentMessage;
        if(msgStatus->processMsgTimer != nullptr)
        {
            cancelAndDelete(msgStatus->processMsgTimer);
        }
    }
//    delete sockets_.removeSocket(tcpSock);
}

bool ExtMecAppBase::getInfoFromServiceRegistry()
{
    if(mp1Socket_ != nullptr)
    {
        EV << "ExtMecAppBase::getting service info from service registry" << endl;
        const char *baseuri = "/example/mec_service_mgmt/v1/services?ser_name=";
        std::string host = mp1Socket_->getRemoteAddress().str()+":"+std::to_string(mp1Socket_->getRemotePort());
        std::string uri;

        // Get information for each required service
        for(auto service : servicesData_)
        {
            uri = baseuri + service->name;
            Http::sendGetRequest(mp1Socket_, host.c_str(), uri.c_str());
            responseCounter_ ++;
        }
        return true;
    }
    else
    {
        EV_INFO << "ExtMecAppBase::error mp1Socket is null" << endl;
        return false;
    }

}

int ExtMecAppBase::findServiceFromSocket(int connId)
{
   inet::TcpSocket *sock_ = check_and_cast<inet::TcpSocket*> (sockets_.getSocketById(connId));
   int index = -1;
   for(int i = 0; i < servicesData_.size(); i ++)
   {
       if(servicesData_[i]->sockid == connId)
       {
          return i;
       }
   }
   return index;
}

//inet::TcpSocket * ExtMecAppBase::findSocketFromService(MecServiceInfo *service)
//{
//
//    for(auto socket : sockets_.getMap())
//    {
//        if(service->host.addr == check_and_cast<inet::TcpSocket*>(socket.second)->getRemoteAddress()
//                && service->host.port == check_and_cast<inet::TcpSocket*>(socket.second)->getRemotePort())
//        {
//            EV << "Socket found" << endl;
//            return check_and_cast<inet::TcpSocket*>(socket.second);
//        }
//    }
//
//    return nullptr;
//
//}
