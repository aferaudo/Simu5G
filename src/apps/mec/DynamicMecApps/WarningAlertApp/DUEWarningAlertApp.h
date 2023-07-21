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

#ifndef __DUEWarningAlertApp_H_
#define __DUEWarningAlertApp_H_

#include "inet/transportlayer/contract/udp/UdpSocket.h"
#include "inet/transportlayer/contract/tcp/TcpSocket.h"
#include "nodes/mec/MECPlatform/MECServices/packets/HttpRequestMessage/HttpRequestMessage.h"
#include "nodes/mec/MECPlatform/MECServices/packets/HttpResponseMessage/HttpResponseMessage.h"
#include "nodes/mec/utils/httpUtils/httpUtils.h"
#include "nodes/mec/MECPlatform/MECServices/packets/HttpMessages_m.h"
#include "inet/networklayer/common/L3Address.h"
#include "inet/networklayer/common/L3AddressResolver.h"

#include "common/binder/Binder.h"

//inet mobility
#include "inet/common/geometry/common/Coord.h"
#include "inet/common/geometry/common/EulerAngles.h"
#include "inet/mobility/contract/IMobility.h"

//WarningAlertPacket
#include "nodes/mec/MECPlatform/MEAppPacket_Types.h"
#include "apps/mec/WarningAlert/packets/WarningAlertPacket_m.h"


using namespace omnetpp;

/**
 * This is a UE app that asks to a Device App to instantiate the MECWarningAlertApp.
 * After a successful response, it asks to the MEC app to be notified when the car
 * enters a circle zone described by x,y center position and the radius. When a danger
 * event arrives, the car colors becomes red.
 *
 * The event behavior flow of the app is:
 * 1) send create MEC app to the Device App
 * 2.1) ACK --> send coordinates to MEC app
 * 2.2) NACK --> do nothing
 * 3) wait for danger events
 * 4) send delete MEC app to the Device App
 */

class DUEWarningAlertApp: public cSimpleModule, public inet::TcpSocket::ICallback
{

    //communication to device app and mec app
    inet::UdpSocket socket;
    inet::TcpSocket amsSocket;
    omnetpp::cQueue completedMessageQueue; // for broken notification

    int size_;
    simtime_t period_;
    int localPort_;
    int deviceAppPort_;
    inet::L3Address deviceAppAddress_;

    char* sourceSimbolicAddress;            //Ue[x]
    char* deviceSimbolicAppAddress_;              //meHost.virtualisationInfrastructure

    // MEC application endPoint (returned by the device app)
    inet::L3Address mecAppAddress_;
    int mecAppPort_;

    std::string mecAppName;

    // mobility informations
    cModule* ue;
    inet::IMobility *mobility;
    inet::Coord position;

    //scheduling
    cMessage *selfStart_;
    cMessage *selfStop_;
    cMessage *selfMecAppStart_;
    cMessage * connectAmsMessage_;
    cMessage * subAmsMessage_;

    inet::L3Address amsAddress;
    int amsPort;

    inet::L3Address coreAddress;

    std::string amsSubscriptionId;
    std::string webHook;

    std::string bufferedData;
    HttpBaseMessage* amsHttpMessage;
    HttpBaseMessage* amsHttpCompleteMessage;

    cModule* pingAppModule;

    static simsignal_t logicTerminated_;

    // uses to write in a log a file
    bool log;

    //core testing
    bool coreTesting_;
    public:
        ~DUEWarningAlertApp();
        DUEWarningAlertApp();

    protected:

        virtual int numInitStages() const override{ return inet::NUM_INIT_STAGES; }
        void initialize(int stage) override;
        virtual void handleMessage(cMessage *msg) override;
        virtual void finish() override;

        void sendStartMEWarningAlertApp();
        void sendMessageToMECApp();
        void sendStopMEWarningAlertApp();

        void handleAckStartMEWarningAlertApp(cMessage* msg);
        void handleInfoMEWarningAlertApp(cMessage* msg);
        void handleAckStopMEWarningAlertApp(cMessage* msg);
        void handleMEAppInfo(cMessage *msg);


        void connect(inet::TcpSocket* socket, const inet::L3Address& address, const int port);
        // Tcp callback
        virtual void socketDataArrived(inet::TcpSocket *socket, inet::Packet *packet, bool urgent) override;
        virtual void socketAvailable(inet::TcpSocket *socket, inet::TcpAvailableInfo *availableInfo) override {};
        virtual void socketEstablished(inet::TcpSocket *socket) override;
        virtual void socketPeerClosed(inet::TcpSocket *socket) override;
        virtual void socketClosed(inet::TcpSocket *socket) override;
        virtual void socketFailure(inet::TcpSocket *socket, int code) override {}
        virtual void socketStatusArrived(inet::TcpSocket *socket, inet::TcpStatusInfo *status) override {}
        virtual void socketDeleted(inet::TcpSocket *socket) override {}
        virtual void allocatePingApp(inet::L3Address mecAppAddress, bool pingMigrated, bool pingCore = false);
        virtual void deallocatePingApp();
};

#endif
