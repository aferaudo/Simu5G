
#include "ExtMecAppBase.h"
#include "apps/mec/MecApps/MecAppBase.h" // we need this to import their structure HttpMessageStatus, thus avoiding to define a new one here.
#include  "apps/mec/MecApps/packets/ProcessingTimeMessage_m.h"

using namespace omnetpp;

ExtMecAppBase::ExtMecAppBase()
{
    // TODO Auto-generated constructor stub
   processMessage_ = nullptr;

}

ExtMecAppBase::~ExtMecAppBase()
{
    // TODO Auto-generated destructor stub
    sockets_.deleteSockets();
    cancelAndDelete(processMessage_);

}

void ExtMecAppBase::initialize(int stage)
{
    if(stage != inet::INITSTAGE_APPLICATION_LAYER)
        return;

    const char *mp1Ip = par("mp1Address");
    mp1Address = inet::L3AddressResolver().resolve(mp1Ip);
    mp1Port = par("mp1Port");

    mecAppId = par("mecAppId"); // FIXME mecAppId is the deviceAppId (it does not change anything, though)
    mecAppIndex_ = par("mecAppIndex");
    requiredRam = par("requiredRam").doubleValue();
    requiredDisk = par("requiredDisk").doubleValue();
    requiredCpu = par("requiredCpu").doubleValue();

    processMessage_ = new cMessage("processedMessage");
}

void ExtMecAppBase::handleMessage(omnetpp::cMessage *msg)
{
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

    delete msg;

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
            delete msg;
        }
    }
}

inet::TcpSocket* ExtMecAppBase::addNewSocket()
{
    inet::TcpSocket* newSocket = new inet::TcpSocket();
    newSocket->setOutputGate(gate("socketOut"));
    newSocket->setCallback(this);

    HttpMessageStatus* msg = new HttpMessageStatus;
    msg->processMsgTimer = new ProcessingTimeMessage("processedHttpMsg");
    msg->processMsgTimer->setSocketId(newSocket->getSocketId());
    newSocket->setUserData(msg);

    sockets_.addSocket(newSocket);
    EV << "ExtMecAppBase::addNewSocket(): added socket with ID: " << newSocket->getSocketId() << endl;
    return newSocket;
}

void ExtMecAppBase::removeSocket(inet::TcpSocket* tcpSock)
{
    HttpMessageStatus* msgStatus = (HttpMessageStatus *) tcpSock->getUserData();
    EV << "ExtMecAppBase::removing socket" << endl;
    while(!msgStatus->httpMessageQueue.isEmpty())
    {
        delete msgStatus->httpMessageQueue.pop();
    }
    if(msgStatus->currentMessage != nullptr )
        delete msgStatus->currentMessage;
    if(msgStatus->processMsgTimer != nullptr)
    {
        cancelAndDelete(msgStatus->processMsgTimer);
    }
    delete sockets_.removeSocket(tcpSock);
}
