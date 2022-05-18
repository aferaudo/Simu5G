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

#ifndef NODES_MEC_MECPLATFORM_MECSERVICES_APPLICATIONMOBILITYSERVICE_RESOURCES_FILTERCRITERIA_H_
#define NODES_MEC_MECPLATFORM_MECSERVICES_APPLICATIONMOBILITYSERVICE_RESOURCES_FILTERCRITERIA_H_
#include<omnetpp.h>
#include "nodes/mec/MECPlatform/MECServices/Resources/AttributeBase.h"
#include "nodes/mec/MECPlatform/MECServices/RNIService/resources/AssociateId.h"
#include "nodes/mec/MECPlatform/MECServices/Resources/FilterCriteriaBase.h"

enum MobilityStatus {INTERHOST_MOVEOUT_TRIGGERED = 0, INTERHOST_MOVEOUT_COMPLETED, INTERHOST_MOVEOUT_FAILED};
static const char* MobilityStatusString [] = {"INTERHOST_MOVEOUT_TRIGGERED", "INTERHOST_MOVEOUT_COMPLETED", "INTERHOST_MOVEOUT_FAILED"};


using namespace omnetpp;

/*
 * This class models the MobilityProcedureSubscription.filterCriteria attribute
 * ETSI MEC 021 v2.2.1
 */

class FilterCriteria : public FilterCriteriaBase{
  private:

    std::vector<AssociateId> associateId_;
    MobilityStatus mobilityStatus_;

    void setMobilityStatusFromString(std::string mobilityStatus);

  public:
    FilterCriteria();
//    FilterCriteria(std::string appInstanceId, MobilityStatus mobilityStatus);
//    FilterCriteria(std::string appInstanceId, std::vector<AssociateId> associateId,MobilityStatus mobilityStatus);
    virtual ~FilterCriteria();

    virtual nlohmann::ordered_json toJson() const;
    virtual bool fromJson(const nlohmann::ordered_json& json);

    void setAssociateId(std::vector<AssociateId> associateId) {associateId_ = associateId;};
    void setMobilityStatus(MobilityStatus mobilityStatus){mobilityStatus_ = mobilityStatus;};

    std::vector<AssociateId> getAssociateId() const {return associateId_;}
    MobilityStatus getMobilityStatus() const {return mobilityStatus_;}
    std::string getMobilityStatusString() const{return MobilityStatusString[mobilityStatus_];}
};

#endif /* NODES_MEC_MECPLATFORM_MECSERVICES_APPLICATIONMOBILITYSERVICE_RESOURCES_FILTERCRITERIA_H_ */
