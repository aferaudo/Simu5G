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

#ifndef __SIMU5G_RESOURCEREGISTERAPP_H_
#define __SIMU5G_RESOURCEREGISTERAPP_H_

#include <omnetpp.h>
#include "inet/common/socket/SocketMap.h"
#include "inet/applications/base/ApplicationBase.h"
#include "inet/transportlayer/contract/tcp/TcpSocket.h"
#include "inet/networklayer/common/L3Address.h"
#include "inet/networklayer/common/L3AddressResolver.h"

using namespace omnetpp;

/**
 * @author Alessandro Calvio
 * @author Angelo Feraudo
 */
class ResourceRegisterApp : public cSimpleModule, public inet::TcpSocket::ICallback
{
    public:
        ResourceRegisterApp();
        virtual ~ResourceRegisterApp() { }

//        virtual TcpSocket *getSocket() { return socket; }
//        virtual void acceptSocket(TcpAvailableInfo *availableInfo);
//
        virtual void socketDataArrived(inet::TcpSocket *socket, inet::Packet *packet, bool urgent) override;
        virtual void socketAvailable(inet::TcpSocket *socket, inet::TcpAvailableInfo *availableInfo) override;
        virtual void socketEstablished(inet::TcpSocket *socket) override {EV << "Connection established" << endl;}
        virtual void socketPeerClosed(inet::TcpSocket *socket) override {}
        virtual void socketClosed(inet::TcpSocket *socket) override {socket->close();}
        virtual void socketFailure(inet::TcpSocket *socket, int code) override {}
        virtual void socketStatusArrived(inet::TcpSocket *socket, inet::TcpStatusInfo *status) override {}
        virtual void socketDeleted(inet::TcpSocket *socket) override {delete serverSocket;}

        
    protected:
        // TODO update

        inet::TcpSocket *serverSocket; // Listen incoming connections
        inet::SocketMap socketMap; //Stores the connections
        inet::L3Address localIPAddress;
        
        int localPort;

        double startTime;

        virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }
        virtual void initialize(int stage) override;
        virtual void handleMessage(cMessage *msg) override;

        
};

#endif
