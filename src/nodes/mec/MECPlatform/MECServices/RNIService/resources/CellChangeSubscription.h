//
//      CellChangeSubscription.h -  Implementation of CellChangeSubscription
//
// Author: Angelo Feraudo (University of Bologna)
//


#ifndef NODES_MEC_MECPLATFORM_MECSERVICES_RNISERVICE_RESOURCES_CELLCHANGESUBSCRIPTION_H_
#define NODES_MEC_MECPLATFORM_MECSERVICES_RNISERVICE_RESOURCES_CELLCHANGESUBSCRIPTION_H_

#include "nodes/mec/MECPlatform/MECServices/Resources/SubscriptionBase.h"
#include "nodes/mec/MECPlatform/MECServices/ApplicationMobilityService/resources/WebsockNotifConfig.h"

class CellChangeSubscription : public SubscriptionBase {
public:
    CellChangeSubscription();
    CellChangeSubscription(unsigned int subId, inet::TcpSocket *socket , const std::string& baseResLocation,  std::set<omnetpp::cModule*>& eNodeBs);

    virtual ~CellChangeSubscription();

    virtual void sendSubscriptionResponse() override;
    virtual void sendNotification(EventNotification *event) override;
    virtual EventNotification* handleSubscription()override;

    virtual bool fromJson(const nlohmann::ordered_json& json) override;
    virtual nlohmann::ordered_json toJson() const override;


private:
    bool requestTestNotification_;
    WebsockNotifConfig websockNotifConfig_;

};

#endif /* NODES_MEC_MECPLATFORM_MECSERVICES_RNISERVICE_RESOURCES_CELLCHANGESUBSCRIPTION_H_ */
