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

#ifndef _MOBILITYPROCEDURENOTIFICATION_H_
#define _MOBILITYPROCEDURENOTIFICATION_H_
#include <omnetpp.h>
#include "nodes/mec/MECPlatform/MECServices/Resources/TimeStamp.h"
#include "nodes/mec/MECPlatform/MECServices/Resources/NotificationBase.h"
#include "MobilityProcedureSubscription.h"
#include "nodes/mec/utils/MecCommon.h"
#include "TargetAppInfo.h"



using namespace omnetpp;

class MobilityProcedureNotification : public NotificationBase{


 private:
    std::string appInstanceId_; // we use this parameter for dynamic case
    MobilityStatus mobilityStatus; //optional: no
    TargetAppInfo targetAppInfo; //optional: yes

 public:
    MobilityProcedureNotification();
//    MobilityProcedureNotification(MobilityStatus ms, std::vector<AssociateId> ids);

    virtual ~MobilityProcedureNotification();

    virtual nlohmann::ordered_json toJson() const override;
    virtual bool fromJson(const nlohmann::ordered_json& json) override;
    virtual EventNotification* handleNotification(FilterCriteriaBase *filters, bool noCheck=false) override;

    //set methods
    void setMobilityStatus(MobilityStatus m) {mobilityStatus = m;};
    void setTargetAppInfo(TargetAppInfo target){targetAppInfo = target;};
    void setLinks(std::string links) {links_ = links;};


    // get methods
    std::string getMobilityStatusString() const {return MobilityStatusString[mobilityStatus];};
    std::string getAppInstanceId() const {return appInstanceId_;};


};

#endif /* _MOBILITYPROCEDURENOTIFICATION_H_ */
