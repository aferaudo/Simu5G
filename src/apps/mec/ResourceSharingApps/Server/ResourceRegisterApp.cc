//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

/*
 *
 * @author Alessandro Calvio
 * @author Angelo Feraudo
 *
 * */

#include "ResourceRegisterApp.h"

Define_Module(ResourceRegisterApp);

ResourceRegisterApp::ResourceRegisterApp()
{
    serverSocket = nullptr;
}

void ResourceRegisterApp::initialize(int stage)
{
    EV << "ResourceRegisterApp::initialize stage " << stage << endl;
    if (stage == inet::INITSTAGE_LOCAL)
    {
        // Initialise lists and other parameters
        EV << "ResourceRegisterApp::initialize" << endl;
        startTime = par("startTime");

        localPort = par("localPort");
    }
    if(stage != inet::INITSTAGE_APPLICATION_LAYER)
       return;



    const char *localAddress = par("localAddress");
    EV << "ResourceRegisterApp::initialize - local Address: " << localAddress << " port: " << localPort << endl;
    localIPAddress = inet::L3AddressResolver().resolve(localAddress);
    EV << "ResourceRegisterApp::initialize - local Address resolved: "<< localIPAddress << endl;
    serverSocket = new inet::TcpSocket();
    serverSocket->setOutputGate(gate("socketOut"));
    serverSocket->bind(localIPAddress, localPort);
    serverSocket->setCallback(this); //set this as callback



    cMessage *msg = new cMessage("listen");
    scheduleAt(simTime()+startTime, msg);


}

void ResourceRegisterApp::handleMessage(cMessage *msg)
{
    if(msg->isSelfMessage() && strcmp(msg->getName(), "listen") == 0)
    {
        // Start listening
        EV << "ResourceRegisterApp::handleMessage - listening on this Address: "<< localIPAddress << " and this port: " << localPort << endl;
        serverSocket->listen();
        delete msg;
    }
    else{
        if(msg->arrivedOn("socketIn"))
        {
            // TODO Manage multiple sockets
            ASSERT(serverSocket && serverSocket->belongsToSocket(msg));
            serverSocket->processMessage(msg);
        }
        else
            throw cRuntimeError("Unknown message");
    }
}

void ResourceRegisterApp::socketDataArrived(inet::TcpSocket *socket, inet::Packet *packet, bool urgent){

    EV << "ResourceRegisterApp::Received packet on socket: " << packet->peekData() << endl;
}

void ResourceRegisterApp::socketAvailable(inet::TcpSocket *socket, inet::TcpAvailableInfo *availableInfo){

    auto newSocket = new inet::TcpSocket(availableInfo);
    newSocket->setOutputGate(gate("socketOut")); //gate connection

    EV << "ResourceRegisterApp::Storing socket " << endl;

    socketMap.addSocket(newSocket);
    serverSocket->accept(availableInfo->getNewSocketId());
    EV << "ResourceRegisterApp::SocketMap size " << socketMap.size() << endl;


}
