/*
 * MecAppDyn.h
 *
 *  Created on: Feb 15, 2022
 *      Author: simu5g
 */

#ifndef APPS_MEC_MECAPPS_DYNAMIC_MECAPPBASEDYN_H_
#define APPS_MEC_MECAPPS_DYNAMIC_MECAPPBASEDYN_H_

#include "apps/mec/MecApps/MecAppBase.h"
#include "apps/mec/ViApp/VirtualisationInfrastructureApp.h"

class  MecAppBaseDyn : public MecAppBase
{
    protected:
        VirtualisationInfrastructureApp* vi;


    public:
        MecAppBaseDyn();
        virtual ~MecAppBaseDyn();

    protected:
        virtual void initialize(int stage) override;

        /* Method to be implemented for real MEC apps */
        virtual void handleSelfMessage(omnetpp::cMessage *msg) override {};
        virtual void handleServiceMessage() override = 0;
        virtual void handleMp1Message() override = 0;
        virtual void handleAmsMessage() override{};
        virtual void handleStateMessage() override{};
        virtual void handleUeMessage(omnetpp::cMessage *msg) override {};
        virtual void established(int connId) override = 0;

        /* inet::TcpSocket::CallbackInterface callback methods */
        virtual void socketDataArrived(inet::TcpSocket *socket, inet::Packet *msg, bool urgent) override;


};


#endif /* APPS_MEC_MECAPPS_DYNAMIC_MECAPPBASEDYN_H_ */
