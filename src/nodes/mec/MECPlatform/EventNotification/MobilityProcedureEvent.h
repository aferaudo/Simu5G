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

#ifndef NODES_MEC_MECPLATFORM_EVENTNOTIFICATION_MOBILITYPROCEDUREEVENT_H_
#define NODES_MEC_MECPLATFORM_EVENTNOTIFICATION_MOBILITYPROCEDUREEVENT_H_
#include <omnetpp.h>
#include "EventNotification.h"
#include "nodes/mec/utils/MecCommon.h"
#include "nodes/mec/MECPlatform/MECServices/ApplicationMobilityService/resources/FilterCriteria.h"
#include "nodes/mec/MECPlatform/MECServices/RNIService/resources/AssociateId.h"
#include "nodes/mec/MECPlatform/MECServices/ApplicationMobilityService/resources/TargetAppInfo.h"
#include "nodes/mec/MECPlatform/MECServices/ApplicationMobilityService/resources/LinkType.h"
#include "nodes/mec/MECPlatform/MECServices/ApplicationMobilityService/resources/MobilityProcedureNotification.h"

using namespace omnetpp;

class MobilityProcedureEvent : public EventNotification{

  private:
//    std::string appInstanceId_; // needed for dynamic case
//    MobilityStatus mobilityStatus_; // identifies the event generated
//    std::vector<AssociateId> associateId_; // id of the ue that is moving (this is useful in case of RNI subscription)
//    TargetAppInfo targetAppInfo_; //optional: yes
//    LinkType _links; // optional: no - according to etsi 021 v2.2.1
    MobilityProcedureNotification *notification_;

  public:
    MobilityProcedureEvent();
    MobilityProcedureEvent(const std::string& type);
    MobilityProcedureEvent(const std::string& type, const int& subId);

    virtual ~MobilityProcedureEvent();

//    SockAddr getNewAddr() const {return newAddr_;};
//    std::string getMobilityStatusString(){return MobilityStatusString[mobilityStatus_];}
//
//    void setTargetAppInfo(TargetAppInfo t){targetAppInfo_ = t;};
//    void setMobilityStatus(MobilityStatus mobilityStatus){mobilityStatus_ = mobilityStatus;};
//    void setAppInstanceId(std::string appInstanceId){appInstanceId_ = appInstanceId;};
//    void setLinks(std::string links) {_links = links;};
    void setMobilityProcedureNotification( MobilityProcedureNotification * notification){notification_ = notification;};

    MobilityProcedureNotification * getMobilityProcedureNotification()const {return notification_;} ;

};

#endif /* NODES_MEC_MECPLATFORM_EVENTNOTIFICATION_MOBILITYPROCEDUREEVENT_H_ */
