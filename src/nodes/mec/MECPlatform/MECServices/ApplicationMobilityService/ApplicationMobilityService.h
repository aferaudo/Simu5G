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

#ifndef __SIMU5G_APPLICATIONMOBILITYSERVICE_H_
#define __SIMU5G_APPLICATIONMOBILITYSERVICE_H_

#include <omnetpp.h>
#include <string>
#include "nodes/mec/MECPlatform/MECServices/MECServiceBase/MecServiceBase.h"

// Resources needed by the service
#include "resources/ApplicationMobilityResource.h"
#include "resources/MobilityProcedureSubscription.h"
#include "resources/MobilityProcedureNotification.h"
#include "resources/FilterCriteria.h"
#include "nodes/mec/MECPlatform/MECServices/Resources/NotificationBase.h"


using namespace omnetpp;

/**
 * Authors:
 * Alessandro Calvio
 * Angelo Feraudo
 *
 *
 *  */
struct SubscriptionLinkList
{
    LinkType self;
    std::vector<std::string> subscriptionType; // 0 = RESERVED, 1 = MOBILITY_PORCEDURE, 2 = ADJACENT_APPINDO
};

class ApplicationMobilityService : public MecServiceBase
{
  std::string baseUriSerDer_;
  std::string callbackUri_; // uri used to receive notification from RNI service
  int applicationServiceIds;
  ApplicationMobilityResource *registrationResources_;
  // id = id device
  // list of subscription for that device
  std::map<std::string, SubscriptionLinkList> subscriptionLinkList; // Not used so far..

  public:
    ApplicationMobilityService();
    ~ApplicationMobilityService();

  protected:
    virtual void initialize(int stage) override;
    virtual void finish() override {};
    virtual void handleMessage(cMessage *msg) override;

    virtual void handleGETRequest(const HttpRequestMessage *currentRequestMessageServed, inet::TcpSocket* socket) override;
    virtual void handlePOSTRequest(const HttpRequestMessage *currentRequestMessageServed, inet::TcpSocket* socket)   override;
    virtual void handlePUTRequest(const HttpRequestMessage *currentRequestMessageServed, inet::TcpSocket* socket)    override;
    virtual void handleDELETERequest(const HttpRequestMessage *currentRequestMessageServed, inet::TcpSocket* socket) override;

    void handleSubscriptionRequest(SubscriptionBase *subscription, inet::TcpSocket* socket, const nlohmann::ordered_json& request);
    void handleNotificationCallback(const nlohmann::ordered_json& request);


    /*
     * This method is called for every element in the subscriptions_ queue.
     */
    virtual bool manageSubscription() override;
  private:
    virtual void printAllSubscription();
};

#endif
