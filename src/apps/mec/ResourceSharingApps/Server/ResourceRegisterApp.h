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
#include <mutex>

// Inet
#include "inet/common/socket/SocketMap.h"
#include "inet/networklayer/common/L3Address.h"
#include "inet/networklayer/common/L3AddressResolver.h"

#include "apps/mec/ResourceSharingApps/PubSub/Publisher/PublisherBase.h"

// ResourceDescriptor struct
#include "nodes/mec/utils/MecCommon.h"


using namespace omnetpp;

/**
 * @author Alessandro Calvio
 * @author Angelo Feraudo
 */

struct ClientEntry
{
    // TODO
    // This structure will be used to dynamically change
    // the kind of reward available
    int clientId;
    inet::L3Address ipAddress;
    ResourceDescriptor resources;
    std::string reward;
    int viPort;
};

class ResourceRegisterThread;

class ResourceRegisterApp : public PublisherBase
{
    // Name of reward: reward
    // for now rewards are represented by integer
    // TODO Add dynamic rewards by using ned files
    std::map <std::string, int> rewardMap_;

    // Map with client resource info
    // key = client id
    // value = ClientEntry;
//    std::map <int, ClientEntry> availableResources_;

    /*
     * Map containing the clientid and its selected reward
     */
    std::map <int, std::string> clientRewards_;

    std::string baseUri_;
    std::string host_;

    std::mutex mtx_write; // Maybe not needed


    public:
        ResourceRegisterApp();
        ~ResourceRegisterApp();

        //set
        virtual void insertClientEntry(ClientEntry c);
        virtual void deleteClientEntry(int clientId);

        // get
        virtual std::string getBaseUri(){return baseUri_;}
        virtual std::map<std::string, int> getRewardMap() {return rewardMap_;}
//        virtual std::map<int, ClientEntry> getAvailableResources() {return availableResources_;}
        virtual std::map<int, std::string> getClientRewards() {return clientRewards_;}

        // other methods
        virtual void removeThread(ResourceRegisterThread *thread);
        
    protected:

        inet::TcpSocket *serverSocket; // Listen incoming connections
        //inet::TcpSocket *vimSocket; //vim-server socket (used to sed new car registrations)
        inet::SocketMap socketMap; //Stores the connections
        inet::L3Address localIPAddress;
        
        typedef std::set<ResourceRegisterThread *> ThreadSet;
        ThreadSet threadSet;


        int localPort;
        simtime_t startTime;

        virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }
        virtual void initialize(int stage) override;
        //virtual void handleMessage(cMessage *msg) override;


        // Other methods
        virtual void initRewardSystem(){EV << "To implement" << endl;};
        virtual void printSelectedRewards();

        // ApplicationBase Methods (AppliacationBase extends cSimpleModule and ILyfecycle (used to support application lyfecycle))
        virtual void handleMessageWhenUp(omnetpp::cMessage *msg) override;
        virtual void handleStartOperation(inet::LifecycleOperation *operation) override;
        virtual void handleStopOperation(inet::LifecycleOperation *operation) override;
        virtual void handleCrashOperation(inet::LifecycleOperation *operation) override;
        virtual void finish() override;
        virtual void refreshDisplay() const override;


        // Callback methods
        //virtual void socketDataArrived(inet::TcpSocket *socket, inet::Packet *packet, bool urgent) override {throw omnetpp::cRuntimeError("ResourceRegisterApp::Unexpected data"); };
        virtual void socketAvailable(inet::TcpSocket *socket, inet::TcpAvailableInfo *availableInfo) override;
        virtual void socketEstablished(inet::TcpSocket *socket) override {EV << "ResourceRegisterApp::Connection established" << endl;}
        virtual void socketPeerClosed(inet::TcpSocket *socket) override {}
        virtual void socketClosed(inet::TcpSocket *socket) override {EV << "ResourceRegisterApp::SocketClosed callback - not managed" << endl;}
        virtual void socketFailure(inet::TcpSocket *socket, int code) override {}
        virtual void socketStatusArrived(inet::TcpSocket *socket, inet::TcpStatusInfo *status) override {}
        virtual void socketDeleted(inet::TcpSocket *socket) override {EV << "ResourceRegisterApp::Socket " << std::to_string(socket->getSocketId()) << " deleted!" << endl;}

        // Abstract methods
        virtual void publish(const char* type, const char *body, int id=-1) override;
};

#endif
