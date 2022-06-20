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

#include "nodes/mec/MECPlatform/MECServices/MECServiceBase/SocketManager.h"

#include "common/utils/utils.h"
#include "nodes/mec/MECPlatform/MECServices/MECServiceBase/MecServiceBase.h"
#include <iostream>
#include "nodes/mec/utils/httpUtils/httpUtils.h"
#include "nodes/mec/MECPlatform/MECServices/packets/HttpRequestMessage/HttpRequestMessage.h"


using namespace omnetpp;
Register_Class(SocketManager);
Define_Module(SocketManager);


void SocketManager::dataArrived(inet::Packet *msg, bool urgent){
    EV << "SocketManager::dataArrived" << endl;
    msg->removeControlInfo();

    inet::b offset = inet::b(0);
    int idx = 0;
    std::vector<uint8_t> bytes =  msg->dup()->peekDataAsBytes()->getBytes();
    auto chunkbytes =  msg->dup()->peekDataAsBytes();
    EV << "SocketManager::dataArrived modified - maxdata : " << msg->getDataLength() << endl;
    int i = 0;
//    std::string packet;
    while(offset < msg->getDataLength()){
        auto chunk = msg->peekAt(offset)->dupShared();

        if(chunk->getChunkLength() > inet::b(0)){
            auto length = chunk->getChunkLength();

            // Logic
            std::vector<uint8_t> interesting_bytes = chunkbytes->peek<inet::BytesChunk>(offset, length)->getBytes();
            std::string interesting_packet(interesting_bytes.begin(), interesting_bytes.end());
//            if(i==0){
//                packet = interesting_packet;
//            }
            i++;
            EV << "SocketManager::dataArrived modified - payload : " << interesting_packet << endl;

            // End Logic

            offset += length;
            EV << "SocketManager::dataArrived modified - new offset : " << offset << endl;
        }
    }

    EV << "SocketManager::dataArrived - payload length: " << bytes.size() << endl;
    std::string packet(bytes.begin(), bytes.end());
    EV << "SocketManager::dataArrived - payload : " << packet << endl;

    /**
     * This block of code inserts fictitious requests to load the server without the need to
     * instantiate and manage MEC apps. A  fictitious request contains the number of requests
     * to insert in the request queue:
     *    BulkRequest: #reqNum
     */
    if(packet.rfind("BulkRequest", 0) == 0)
    {
        EV << "Bulk Request ";
        std::string size = lte::utils::splitString(packet, ": ")[1];
        int requests = std::stoi(size);
        if(requests < 0)
          throw cRuntimeError("Number of request must be non negative");
        EV << " of size "<< requests << endl;
        if(requests == 0)
        {
            return;
        }
        for(int i = 0 ; i < requests ; ++i)
        {
          if(i == requests -1)
          {
              HttpRequestMessage *req = new HttpRequestMessage();
              req->setLastBackGroundRequest(true);
              req->setSockId(sock->getSocketId());
              req->setState(CORRECT);
              service->newRequest(req); //use it to send back response message (only for the last message)
          }
          else
          {
              HttpRequestMessage *req = new HttpRequestMessage();
              req->setBackGroundRequest(true);
              req->setSockId(sock->getSocketId());
              req->setState(CORRECT);
              service->newRequest(req);
          }
        }
        delete msg;
        return;
    }
    // ########################

    bool res = true;
    EV_INFO << "there are " << completedMessageQueue.getLength() << " messages" << endl;
    std::cout << "Buffer prima " << bufferedData << endl;
    Http::parseReceivedMsg(sock->getSocketId(), packet, completedMessageQueue, &bufferedData, &currentHttpMessage);
    std::cout << "Buffer dopo" << bufferedData << endl;
    if(currentHttpMessage == nullptr){
        std::cout << "nullptr message" << endl;
    }else{
        std::cout << (*currentHttpMessage).getBody() << endl;
    }
    EV_INFO << "found " << completedMessageQueue.getLength() << " messages" << endl;
    std::cout << "found " << completedMessageQueue.getLength() << " messages" << endl;
    if(res)
    {

        while(completedMessageQueue.getLength() > 0){
            HttpBaseMessage* message = check_and_cast<HttpBaseMessage*>(completedMessageQueue.pop());

            std::cout << "analyzing " << message->getBody() << endl;
            message->setSockId(sock->getSocketId());
            std::cout << "analyzing 2" << endl;
            if(message->getType() == REQUEST)
            {
                std::cout << "analyzing 3" << endl;
                service->emitRequestQueueLength();
                message->setArrivalTime(simTime());
                service->newRequest(check_and_cast<HttpRequestMessage*>(message));
                std::cout << "analyzing 4" << endl;
            }
            else
            {
                delete message;
            }
//            if(currentHttpMessage != nullptr)
//            {
//              currentHttpMessage = nullptr;
//            }
        }

        std::cout << "analyzing 5 - " << completedMessageQueue.getLength() << endl;
//        completedMessageQueue.clear();
        std::cout << "analyzing 55" << endl;
    }
    std::cout << "analyzing 56" << endl;
    delete msg;
    std::cout << "analyzing 6" << endl;
    return;
}

void SocketManager::established(){
    EV_INFO << "New connection established " << endl;

    buffers[sock->getSocketId()] = std::string();
}

void SocketManager::peerClosed()
{
    std::cout<<"Closed connection from: " << sock->getRemoteAddress()<< std::endl;
    sock->setState(inet::TcpSocket::PEER_CLOSED);
    service->removeSubscritions(sock->getSocketId()); // if any

    //service->removeConnection(this); //sock->close(); // it crashes when mec app is deleted  with ->deleteModule FIXME

    // FIXME resolve this comments
//    EV << "MeServiceBase::removeConnection"<<endl;
//    // remove socket
//    inet::TcpSocket * sock = check_and_cast< inet::TcpSocket*>( socketMap.removeSocket(connection->getSocket()));
//    sock->close();
//    delete sock;
//    // remove thread object
//    threadSet.erase(connection);
//    connection->deleteModule();
}

void SocketManager::closed()
{
    std::cout <<"Removed socket of: " << sock->getRemoteAddress() << " from map" << std::endl;
    sock->setState(inet::TcpSocket::CLOSED);
    service->removeSubscritions(sock->getSocketId());
    service->closeConnection(this);
}

void SocketManager::failure(int code)
{
    std::cout <<"Socket of: " << sock->getRemoteAddress() << " failed. Code: " << code << std::endl;
    service->removeSubscritions(sock->getSocketId());
    service->removeConnection(this);
}
