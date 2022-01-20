/*
 * ResourceRegisterThreadBase.h
 *
 *  Created on 2022
 */

#ifndef APPS_MEC_RESOURCESHARINGAPPS_SERVER_RESOURCEREGISTERTHREADBASE_H_
#define APPS_MEC_RESOURCESHARINGAPPS_SERVER_RESOURCEREGISTERTHREADBASE_H_

#include <omnetpp.h>

#include "nodes/mec/MECPlatform/MECServices/packets/HttpMessages_m.h"
#include "nodes/mec/utils/httpUtils/httpUtils.h"
#include "nodes/mec/MECPlatform/MECServices/packets/HttpRequestMessage/HttpRequestMessage.h"

#include "inet/transportlayer/contract/tcp/TcpSocket.h"




class ResourceRegisterThreadBase : public omnetpp::cSimpleModule, public inet::TcpSocket::ICallback {


  protected:
    HttpBaseMessage* currentHttpMessage; // current HttpRequest
    std::string buffer;

    inet::TcpSocket *sock; // ptr into socketMap managed by TcpServerHostApp

    // internal: TcpSocket::ICallback methods
    virtual void socketDataArrived(inet::TcpSocket *socket, inet::Packet *msg, bool urgent) override { dataArrived(msg, urgent); }
    virtual void socketAvailable(inet::TcpSocket *socket, inet::TcpAvailableInfo *availableInfo) override { socket->accept(availableInfo->getNewSocketId()); }
    virtual void socketEstablished(inet::TcpSocket *socket) override { established(); }
    virtual void socketPeerClosed(inet::TcpSocket *socket) override { peerClosed(); }
    virtual void socketClosed(inet::TcpSocket *socket) override { sock->close(); }
    virtual void socketFailure(inet::TcpSocket *socket, int code) override { failure(code); }
    virtual void socketStatusArrived(inet::TcpSocket *socket, inet::TcpStatusInfo *status) override { statusArrived(status); }
    virtual void socketDeleted(inet::TcpSocket *socket) override { if (socket == sock) sock = nullptr; }

    virtual void refreshDisplay() const override {};

    //HttpMethods API
    virtual void handleGETRequest(const HttpRequestMessage *currentRequestMessageServed, inet::TcpSocket* socket) { EV << "To Be implemented"; };
    virtual void handlePOSTRequest(const HttpRequestMessage *currentRequestMessageServed, inet::TcpSocket* socket) { EV << "To Be implemented"; };
    virtual void handlePUTRequest(const HttpRequestMessage *currentRequestMessageServed, inet::TcpSocket* socket)    { EV << "To Be implemented"; };
    virtual void handleDELETERequest(const HttpRequestMessage *currentRequestMessageServed, inet::TcpSocket* socket) { EV << "To Be implemented"; };


  public:

    ResourceRegisterThreadBase() { sock = nullptr; currentHttpMessage = nullptr; }
    virtual ~ResourceRegisterThreadBase() { delete sock; }

    // internal: called by TcpServerHostApp after creating this module
    virtual void init(inet::TcpSocket *socket) { sock = socket; }

    /*
     * Returns the socket object
     */
    virtual inet::TcpSocket *getSocket() { return sock; }

    /*
     * Returns pointer to the host module
     */
    virtual HttpBaseMessage *getCurrentHttpMessage() { return currentHttpMessage; }

    /**
     * Called when connection is established. To be redefined.
     */
    virtual void established();

    /*
     * Called when a data packet arrives. To be redefined.
     */
    virtual void dataArrived(inet::Packet *msg, bool urgent);

    /*
     * Called when a timer (scheduled via scheduleAt()) expires. To be redefined.
     */
    virtual void timerExpired(omnetpp::cMessage *timer) {};

    /*
     * Called when the client closes the connection. By default it closes
     * our side too, but it can be redefined to do something different.
     */
    virtual void peerClosed();

    /*
     * Called when the connection breaks (TCP error). By default it deletes
     * this thread, but it can be redefined to do something different.
     */
    virtual void failure(int code); // TODO to implement

    /*
     * Called when a status arrives in response to getSocket()->getStatus().
     * By default it deletes the status object, redefine it to add code
     * to examine the status.
     */
    virtual void statusArrived(inet::TcpStatusInfo *status) {}
};


#endif /* APPS_MEC_RESOURCESHARINGAPPS_SERVER_RESOURCEREGISTERTHREADBASE_H_ */
