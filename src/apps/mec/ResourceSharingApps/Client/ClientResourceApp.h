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

#ifndef __SIMU5G_CLIENTRESOURCEAPP_H_
#define __SIMU5G_CLIENTRESOURCEAPP_H_

#include <omnetpp.h>
#include <iostream>
#include "inet/transportlayer/contract/tcp/TcpSocket.h"
#include "inet/networklayer/common/L3Address.h"
#include "inet/networklayer/common/L3AddressResolver.h"

using namespace omnetpp;

/**
 * TODO - Generated class
 */
class ClientResourceApp : public cSimpleModule
{
  protected:

    inet::TcpSocket tcpSocket;

    inet::L3Address localIPAddress;
    int localPort;

    inet::L3Address destIPAddress;
    int destPort;

    double startTime;
    double stopTime;


    virtual void initialize(int stage) override;
    virtual void handleMessage(cMessage *msg) override;

    virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }

    /*Other methods*/
    virtual void connectToSRR();
    virtual void handleSelfMessage(cMessage *msg);


  
  public:
    ClientResourceApp();
    virtual ~ClientResourceApp();
};

#endif
