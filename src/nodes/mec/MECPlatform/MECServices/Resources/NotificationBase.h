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

#ifndef NODES_MEC_MECPLATFORM_MECSERVICES_RESOURCES_NOTIFICATIONBASE_H_
#define NODES_MEC_MECPLATFORM_MECSERVICES_RESOURCES_NOTIFICATIONBASE_H_
#include <omnetpp.h>

#include "AttributeBase.h"
#include "TimeStamp.h"
#include "nodes/mec/MECPlatform/EventNotification/EventNotification.h"
#include "FilterCriteriaBase.h"
#include "nodes/mec/MECPlatform/MECServices/RNIService/resources/AssociateId.h"

/*
 * This abstract class maintains common information of Application Mobility Service notifications:
 * - MobilityProcedureNotification
 * - AdjacentAppInfoNotification
 * ExpiryNotification should be modeled as an internal timer
 * */
class NotificationBase : public AttributeBase{
  protected:
    std::string notificationType_;
    TimeStamp timestamp_;
    std::vector<AssociateId> associateId_;

  public:
    NotificationBase();
    virtual ~NotificationBase();

    virtual nlohmann::ordered_json toJson() const = 0;
    virtual bool fromJson(const nlohmann::ordered_json& json) = 0;
    virtual EventNotification* handleNotification(FilterCriteriaBase *filters, bool noCheck=false) = 0;

    void setTimeStamp(TimeStamp t){timestamp_ = t;};
    std::string getNotificationType() const{return notificationType_;}
    TimeStamp getTimestamp(){return timestamp_;}

    std::vector<AssociateId> getAssociateId() const{return associateId_;};
    void setAssociateId(std::vector<AssociateId> associateId){associateId_ = associateId;};

};

#endif /* NODES_MEC_MECPLATFORM_MECSERVICES_RESOURCES_NOTIFICATIONBASE_H_ */
