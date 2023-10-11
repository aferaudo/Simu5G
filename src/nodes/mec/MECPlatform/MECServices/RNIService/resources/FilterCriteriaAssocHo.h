//
//      FilterCriteriaAssocHo.h -  Implementation of FilterCriteriaAssocHo
//
// Author: Angelo Feraudo (University of Bologna)
//


#ifndef NODES_MEC_MECPLATFORM_MECSERVICES_RNISERVICE_RESOURCES_FILTERCRITERIAASSOCHO_H_
#define NODES_MEC_MECPLATFORM_MECSERVICES_RNISERVICE_RESOURCES_FILTERCRITERIAASSOCHO_H_

#include "nodes/mec/MECPlatform/MECServices/Resources/FilterCriteriaBase.h"
#include "nodes/mec/MECPlatform/MECServices/RNIService/resources/AssociateId.h"
#include "Ecgi.h"


enum HoStatus {INVALID, IN_PREPARATION, IN_EXECUTION, COMPLETED, REJECTED, CANCELLED};
static const char* hoStatusString [] = {"INVALID", "IN_PREPARATION", "IN_EXECUTION", "COMPLETED", "REJECTED", "CANCELLED"};

/*
 * This class models the CellChangeSubscription.filterCriteriaAssocHo attribute
 * ETSI MEC 012 v2.2.1
 */

class FilterCriteriaAssocHo : public FilterCriteriaBase{
public:
    FilterCriteriaAssocHo();
    virtual ~FilterCriteriaAssocHo();
    virtual nlohmann::ordered_json toJson() const override;
    virtual bool fromJson(const nlohmann::ordered_json& json) override;

    void setAssociateId(std::vector<AssociateId> associateId) {associateId_ = associateId;};
    void setHoStatus(std::vector<HoStatus> hoStatus){hoStatus_ = hoStatus;};
    void setHoStatusEcgi(std::vector<Ecgi> ecgi){ecgi_ = ecgi;};

    std::vector<AssociateId> getAssociateId() const {return associateId_;}
    std::vector<HoStatus> getHoStatus() const {return hoStatus_;}
    std::vector<Ecgi> getHoStatusEcgi() const {return ecgi_;}

    static std::string getHoStatusString(HoStatus hoStatus) {return hoStatusString[hoStatus];}
    static HoStatus getHoStatusFromString(std::string hoStatus);

private:
    std::vector<AssociateId> associateId_;
    std::vector<HoStatus> hoStatus_;
    std::vector<Ecgi> ecgi_;
};

#endif /* NODES_MEC_MECPLATFORM_MECSERVICES_RNISERVICE_RESOURCES_FILTERCRITERIAASSOCHO_H_ */
