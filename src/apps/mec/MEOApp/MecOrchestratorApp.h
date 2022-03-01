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

#ifndef __SIMU5G_MECORCHESTRATORAPP_H_
#define __SIMU5G_MECORCHESTRATORAPP_H_

#include <omnetpp.h>

// inet
#include "inet/applications/base/ApplicationBase.h"
#include "inet/networklayer/common/L3Address.h"
#include "inet/networklayer/common/L3AddressResolver.h"
#include "inet/transportlayer/contract/udp/UdpSocket.h"


// Registration Packet
#include "apps/mec/MEOApp/Messages/RegistrationPkt_m.h"


using namespace omnetpp;

/**
 * author:
 * Alessandro Calvio
 * Angelo Feraudo
 */

enum ResponseResult {NO_VALUE, TRUE, FALSE};
enum MecHostInfo {INCOMPLETE, COMPLETE};

struct MECHostDescriptor
{
    int mecHostId; // TODO do we need this?
    int vimPort = -1; // default value -1 (no port available)
    int mepmPort = -1; // default value -1 (no port available)
    inet::L3Address mecHostIp;
};

struct MECHostResponseEntry
{
    ResponseResult vimRes;
    ResponseResult mepmRes;
    MECHostDescriptor mecHostDesc;
};

// TODO Do we need this infrastructure?
struct MECHostRegistrationEntry
{
    MecHostInfo information = INCOMPLETE; // default value INCOMPLETE (one of MECHost port is not available)
    MECHostDescriptor mecHostDesc;
    std::string toString()
    {
        return "MECHost ID: " + std::to_string(mecHostDesc.mecHostId)
                + ", information availability: " + std::to_string(information)
                + ", ipAddress: " + mecHostDesc.mecHostIp.str() + ", vimPort: "
                + std::to_string(mecHostDesc.vimPort) + ", mepmPort: " + std::to_string(mecHostDesc.mepmPort);

    }
};

struct mecAppMapEntry
{
    int contextId;
    std::string appDId;
    std::string mecAppName;
    std::string mecAppIsntanceId;
    int mecUeAppID;         //ID

    MECHostDescriptor mecHostDesc; // mechost where the mecApp has been deployed

    std::string ueSymbolicAddres;
    inet::L3Address ueAddress;  //for downstream using UDP Socket
    int uePort;
    inet::L3Address mecAppAddress;  //for downstream using UDP Socket
    int mecAppPort;

    bool isEmulated;

    int lastAckStartSeqNum;
    int lastAckStopSeqNum;

};

class MecOrchestratorApp : public inet::ApplicationBase, public inet::UdpSocket::ICallback
{
  public:
    MecOrchestratorApp () {};
    ~MecOrchestratorApp() {};
  protected:

    // Socket paramters
    int localPort;
    inet::L3Address localIPAddress;
    inet::UdpSocket socket;


    /* Available MECHosts
     * key - mecHost ID
     * value - mecHostDescriptor
     * MEC Hosts may not have all the necessary information
     * to be considered during MECApp deplyment
     * MECHosts have two components:
     * - a MECPlatformManager
     * - A VirtualisationIfastructureManager
     * A MECHost to be considered "valid" need to provide the
     * possibility to communicate with both MEPM and VIM.
     */
    std::map<int, MECHostRegistrationEntry *> mecHosts;

    // Map to match mecAppHosting
    // TODO maybe this is too reductive
    //std::map<int, MECHostDescriptor> meAppLocation;

    /*
     * MAP which takes trace of the serving request
     * request id (typically device app id)
     * List of mechost response waiting
     */
    std::map<int, std::set<MECHostResponseEntry>> requestMap;


    virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    virtual void initialize(int stage) override;

    // ApplicationBase Methods (AppliacationBase extends cSimpleModule and ILyfecycle (used to support application lyfecycle))
    virtual void handleMessageWhenUp(omnetpp::cMessage *msg) override;
    virtual void handleStartOperation(inet::LifecycleOperation *operation) override;
    virtual void handleStopOperation(inet::LifecycleOperation *operation) override {};
    virtual void handleCrashOperation(inet::LifecycleOperation *operation) override {socket.destroy();};
    virtual void finish() override {};
    virtual void refreshDisplay() const override {};


    virtual void socketDataArrived(inet::UdpSocket *socket, inet::Packet *packet) override;
    virtual void socketErrorArrived(inet::UdpSocket *socket, inet::Indication *indication) override {};
    virtual void socketClosed(inet::UdpSocket *socket) override {};


    // other methods
    virtual void handleRegistration(const RegistrationPkt *data);
    virtual void printAvailableMECHosts();
};

#endif
