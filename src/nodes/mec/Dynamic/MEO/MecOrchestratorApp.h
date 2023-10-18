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

// MECOrchestrator interface
#include "nodes/mec/Dynamic/MEO/IMecOrchestrator.h"

// Registration Packet
#include "nodes/mec/Dynamic/MEO/Messages/RegistrationPkt_m.h"

// Application Descriptor class
#include "nodes/mec/MECOrchestrator/ApplicationDescriptor/ApplicationDescriptor.h"

// UALCMP messages
#include "nodes/mec/UALCMP/UALCMPMessages/UALCMPMessages_m.h"
#include "nodes/mec/UALCMP/UALCMPMessages/UALCMPMessages_types.h"
#include "nodes/mec/UALCMP/UALCMPMessages/CreateContextAppMessage.h"
#include "nodes/mec/UALCMP/UALCMPMessages/CreateContextAppAckMessage.h"


// mm4 messages
//#include "apps/mec/MEOApp/Messages/MeoVimPackets_m.h"
//
//// mm3 messages
//#include "apps/mec/MEOApp/Messages/MeoMepmPackets_m.h"

//mm3 and mm4 messages
#include "nodes/mec/Dynamic/MEO/Messages/MeoPackets_m.h"

using namespace omnetpp;

/**
 * This class is based on nodes/mec/MECOrchestrator/MecOrchestrator
 * author:
 * Alessandro Calvio
 * Angelo Feraudo
 *
 * Notes:
 *  - Added network stack for MEC host communications
 *  - UALCMP interactions same as those implemented by unipi team
 */

enum ResponseResult {FALSE, TRUE, NO_VALUE};
enum MecHostInfo {INCOMPLETE, COMPLETE};
struct MECHostDescriptor
{
    int mecHostId;
    int vimPort = -1; // default value -1 (no port available)
    int mepmPort = -1; // default value -1 (no port available)
    inet::L3Address mepmHostIp;
    inet::L3Address vimHostIp;
    double lastAllocation = -1; // time of last allocation request (startMECApp)
    std::string toString() const
    {
        return "MECHost ID: " + std::to_string(mecHostId)
                + ", mepmIpAddress: " + mepmHostIp.str() + ", vimIpAddress: " + vimHostIp.str() + ", vimPort: "
                + std::to_string(vimPort) + ", mepmPort: " + std::to_string(mepmPort) +
                ", last allocation time: " + std::to_string(lastAllocation);

    }
};

struct MECHostResponseEntry
{
    ResponseResult vimRes = NO_VALUE;
    ResponseResult mepmRes = NO_VALUE;
    double requestTime;
    //MECHostDescriptor mecHostDesc;
    int mecHostID;

    std::string toString() const
    {
        return "ResponseEntry::mecHostId: " + std::to_string(mecHostID)
                + ", requestTime: " + std::to_string(requestTime)
                + ", vimRes: " + std::to_string(vimRes)
                + ", mepmRes: " + std::to_string(mepmRes);

    }
};


struct mecApp_s
{
    int contextId;
    std::string appDId;
    std::string mecAppName;
    std::string mecAppIsntanceId;
    int mecUeAppID;         //ID

    MECHostDescriptor *mecHostDesc; // mechost where the mecApp has been deployed - vim and mepm address and port

    std::string ueSymbolicAddres;
    inet::L3Address ueAddress;  //for downstream using UDP Socket
    int uePort;
    inet::L3Address mecAppAddress;  //for downstream using UDP Socket
    int mecAppPort;

    bool isEmulated;

    int lastAckStartSeqNum;
    int lastAckStopSeqNum;

};

struct ResourceRequest
{
    inet::Packet* pktMM3;
    inet::Packet* pktMM4;
    inet::L3Address mepmHostAddress;
    inet::L3Address vimHostAddress;
    int vimPort;
    int mepmPort;
};


class MecOrchestratorApp : public inet::ApplicationBase, public inet::UdpSocket::ICallback, public IMecOrchestrator
{
    /*
     * Storing MECApp information
     * */
    std::map<std::string, ApplicationDescriptor> mecApplicationDescriptors_;

    int contextIdCounter;
    //key = contextId - value mecAppMapEntry
    std::map<int, mecApp_s> meAppMap;

    /* Available MECHosts
     * value - mecHostDescriptor
     * MEC Hosts may not have all the necessary information
     * to be considered during MECApp deployment
     * MECHosts have two components:
     * - a MECPlatformManager
     * - A VirtualisationIfastructureManager
     * A MECHost to be considered "valid" need to provide the
     * possibility to communicate with both MEPM and VIM.
     */
    std::vector<MECHostDescriptor*> mecHosts;

    /*
     * Can we merge this two maps in some way?
     * Map containing pending instantiation messages from UALCMP
     * This map regards only instantiation
     * key: deviceAppId - id of the DeviceAPP
     * value: CreateContextAppMessage message
     */
    std::map<std::string, CreateContextAppMessage*> pendingRequests;

    // Asynchronism management
    /*
     * MAP which takes trace of the requests send to mechostss
     * key: request id (typically device app id), mecHost
     * value: MECHostResponseEntry
     */
    std::map<std::string, std::vector<MECHostResponseEntry*>> responseMap;

    /*
     * Vector used to manage asynchronous requests
     * int: mecHostId
     * double: allocation Time - i.e.when the last app has been allocated in that MECHost
     */
//    std::vector<std::pair<int, double>> lastAllocatedVect;

    std::queue<ResourceRequest*> resourceRequestQueue_;
    cMessage* processResourceRequest_;


  public:
    MecOrchestratorApp ();
    ~MecOrchestratorApp();
    // methods used by the UALCMP
    // both have been left unchanged
    const ApplicationDescriptor* getApplicationDescriptorByAppName(std::string& appName) const override;
    const std::map<std::string, ApplicationDescriptor>* getApplicationDescriptors() const override { return &mecApplicationDescriptors_;}

  protected:

    // Socket paramters
    int localPort;
    inet::L3Address localIPAddress;
    inet::UdpSocket socket;

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
    void handleRegistration(inet::Packet *packet);
    void handleResourceReply(inet::Packet *packet);
    void handleInstantiationResponse(inet::Packet *packet);
    void handleTerminationResponse(inet::Packet *packet);

    /*
     * This method sends to a MECHost two requests:
     *  - service required request (to MEPM via MM3)
     *  - resource required request (to VIM via MM4)
     * */
    void sendSRRequest();

    // utility methods
    void printAvailableMECHosts();
    void printAvailableAppDescs();
    inet::Packet* makeResourceRequestPacket(inet::L3Address dstAddress, int dstPort, std::string deviceAppId, double cpu, double ram, double disk);
    inet::Packet* makeAvailableServiceRequestPacket(inet::L3Address dstAddress, int dstPort, std::string deviceAppId, const ApplicationDescriptor& appDesc);
    void handleUALCMPMessage(cMessage* msg);
    void handleCreateContextMessage(CreateContextAppMessage* contAppMsg);

    // source: nodes/mec/MECOrchestrator/MecOrchestrator.h
    // device app id is used to remove the UALCMP request from the pending request list
    void sendCreateAppContextAck(bool result, unsigned int requestSno, int contextId=-1, const std::string &deviceAppId = std::string(), std::string amsUri = std::string());
    void sendDeleteAppContextAck(bool result, unsigned int requestSno, int contextId = -1);


    /*
     * it
     * */
    void startMECApp(CreateContextAppMessage* contAppMsg, MECHostDescriptor *bestHost);

    /* src: nodes/mec/MECOrchestrator/MecOrchestrator.h
     * handling DELETE_CONTEXT_APP type
     * it calls the method of the MEC platform manager of the MEC host where the MEC app has been deployed
     * to delete the MEC app
     * */
    void stopMECApp(UALCMPMessage* msg);

    /*
     * Method fixed with real message exchange
     * This method selects the most suitable MEC host where to deploy the MEC app.
     * The policies for the choice of the MEC host refer both from computation requirements
     * and required MEC services.
     *
     * The current implementations of the method selects the MEC host based on the availability of the
     * required resources and the MEC host that also runs the required MEC service (if any) has precedence
     * among the others.
     *
     * @param ApplicationDescriptor with the computation and MEC services requirements
     *
     * @return MECHostDescriptor
     */
    //MECHostDescriptor* findBestMecHost(const ApplicationDescriptor& appDesc); // this signature is used for testing
    /*
     * this will be the actual signature of the method
     * it does not return anything, because resource and service requests happen through messages
     * deviceApp identifier - device that made the request
     */
    void findBestMecHost(std::string deviceAppId, const ApplicationDescriptor& appDesc);

    /*
    This method is responsible for deploying a specified MEC application on a specified MEC host (or * all). 
    The deviceAppId parameter is used to identify the device that made the request. 
    The appDesc parameter contains the computation and MEC services requirements of the MEC application to be deployed. 
    The location parameter specifies the MEC host where the MEC application should be deployed.

    The method does not return anything, as the deployment process happens through messages.

    */
    void deployOnSpecifiedMecHost(std::string deviceAppId, const ApplicationDescriptor& appDesc, std::string location);
    
    /*
     * src: nodes/mec/MECOrchestrator/MecOrchestrator.h
     * The list of the MEC app descriptor to be onboarded at initialization time is
     * configured through the mecApplicationPackageList NED parameter.
     * This method loads the app descriptors in the mecApplicationDescriptors_ map
     *
     */
    void onboardApplicationPackages();

    /*
     * src: nodes/mec/MECOrchestrator/MecOrchestrator.h
     * This method loads the app descriptors at runtime.
     *
     * @param ApplicationDescriptor with the computation and MEC services requirements
     *
     * @return ApplicationDescriptor structure of the MEC app descriptor
     */
    const ApplicationDescriptor& onboardApplicationPackage(const char* fileName);


};

#endif
