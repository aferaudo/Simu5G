/*
 * ResourceRegisterThreadBase.cc
 *
 */

#include "ResourceRegisterThreadBase.h"



using namespace omnetpp;

Register_Class(ResourceRegisterThreadBase);
Define_Module(ResourceRegisterThreadBase);


void ResourceRegisterThreadBase::dataArrived(inet::Packet *packet, bool urgent){

    EV << "ResourceRegisterThreadBase::Received packet on socket: " << packet->peekData() << endl;
    std::vector<uint8_t> bytes =  packet->peekDataAsBytes()->getBytes();
    std::string msg(bytes.begin(), bytes.end());

    EV << "ResourceRegisterThreadBase::Packet bytes: " << msg;

    bool res = Http::parseReceivedMsg(msg, &buffer, &currentHttpMessage);
}

void ResourceRegisterThreadBase::established(){
    EV << "To be implemented";
}


void ResourceRegisterThreadBase::failure(int code){
    EV << "To be implemented";
}

void ResourceRegisterThreadBase::peerClosed(){
    EV << "ResourceRegisterThreadBase::Peer closed the connection" << sock->getRemoteAddress() << endl;
    sock->setState(inet::TcpSocket::PEER_CLOSED);
}
