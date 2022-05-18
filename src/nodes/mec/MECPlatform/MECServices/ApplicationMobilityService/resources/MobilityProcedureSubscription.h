/*
 * MobilityProcedureSubscription.h
 *
 *  Created on: May 10, 2022
 *      Author: Alessandro Calvio, Angelo Feraudo
 */

#ifndef _MOBILITYPROCEDURESUBSCRIPTION_H_
#define _MOBILITYPROCEDURESUBSCRIPTION_H_
#include <omnetpp.h>
#include "nodes/mec/MECPlatform/MECServices/Resources/SubscriptionBase.h"
#include "nodes/mec/MECPlatform/MECServices/Resources/TimeStamp.h"
#include "ApplicationMobilityResource.h"
#include "LinkType.h"
#include "FilterCriteria.h"
#include "WebsockNotifConfig.h"

using namespace omnetpp;


class MobilityProcedureSubscription : public SubscriptionBase{
  private:
    //const std::string subscriptionType = "MobilityProcedureSubscription"; //optional: no (Shall be set to "MobilityProcedureSubscription")
    /*
     * - Please be aware that at least one of callbackReference and websockNotifConfig shall be provided by service consumer
     */
    //callbackReference; //optional: yes (since ETSI 021 V2.2.1)
    WebsockNotifConfig websockNotifConfig; //optional:yes
    bool requestTestNotification; // optional: yes (since ETSI 021 V2.2.1)
    LinkType _links; // optional: yes
    // filterCriteria; //optional: no
    TimeStamp expiryDeadline; //optional: yes

  public:
    MobilityProcedureSubscription();
    MobilityProcedureSubscription(unsigned int subId, inet::TcpSocket *socket , const std::string& baseResLocation,  std::set<omnetpp::cModule*>& eNodeBs);
    virtual ~MobilityProcedureSubscription();


    virtual bool fromJson(const nlohmann::ordered_json& json) override;
    virtual void sendSubscriptionResponse() override;
    virtual void sendNotification(EventNotification *event) override;
    virtual EventNotification* handleSubscription()override;

    void set_links(std::string& link) override {_links.setHref(link+"sub"+std::to_string(subscriptionId_));}; //  This is set while preparing the response - hyperlink related to the resource

    virtual void setFilterCriteria(FilterCriteriaBase* filterCriteria) override { filterCriteria_ = static_cast<FilterCriteria*>(filterCriteria);}


    //FilterCriteria getFilterCriteria() const override{return filterCriteria;}


};

#endif /*_MOBILITYPROCEDURESUBSCRIPTION_H_ */
