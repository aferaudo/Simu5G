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

#ifndef __SIMU5G_SENTINELMECAPP_H_
#define __SIMU5G_SENTINELMECAPP_H_

#include <omnetpp.h>
#include "apps/mec/DynamicMecApps/MecAppBase/ExtMecAppBase.h"

using namespace omnetpp;

/**
 * TODO - Generated class
 */
class SentinelMecApp : public ExtMecAppBase
{
  enum SERVICE {
               RNI = 0, // Radio Network Information Service (RNI)
  };

  //UDP socket to communicate with the UeApp
  // TODO maybe we don't need this
  inet::UdpSocket ueSocket;
  int localUePort;

  int localPort;

  std::string subId_;
  std::vector<std::string> serNames;

  // RNI notification parameters
  std::string webHook_;

  protected:
    virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    virtual void initialize(int stage) override;


    virtual void handleSelfMessage(omnetpp::cMessage *msg);
    virtual void handleHttpMessage(int connId);
    virtual void handleReceivedMessage(int sockId, inet::Packet *msg); // this method menage message received on a socket in listening mode
    virtual void handleServiceMessage(int index);
    virtual void handleMp1Message(int connId);
    virtual void handleUeMessage(omnetpp::cMessage *msg);
    virtual void established(int connId);
    virtual void handleTermination(); //delete registrations
    virtual void sendCellChangeSubscription(inet::TcpSocket *socket);
    virtual void sendDeleteCellChangeSubscription(inet::TcpSocket *socket);
    virtual void handleRNIMessage(inet::TcpSocket *socket);

};

#endif
