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

#ifndef NODES_MEC_MECPLATFORM_MECSERVICES_APPLICATIONMOBILITYSERVICE_RESOURCES_TARGETINFO_H_
#define NODES_MEC_MECPLATFORM_MECSERVICES_APPLICATIONMOBILITYSERVICE_RESOURCES_TARGETINFO_H_
#include<omnetpp.h>
#include "nodes/mec/MECPlatform/MECServices/Resources/AttributeBase.h"
#include "inet/networklayer/common/L3Address.h"
#include "inet/networklayer/common/L3AddressResolver.h"
#include "nodes/mec/utils/MecCommon.h"

using namespace omnetpp;
class TargetAppInfo: public AttributeBase {
  private:
    std::string appInstanceId_; //optional no
    std::vector<SockAddr> commInterface_;
  public:
    TargetAppInfo();
    virtual ~TargetAppInfo();
    virtual nlohmann::ordered_json toJson() const;
    virtual bool fromJson(const nlohmann::ordered_json& json);

    void setAppInstanceId(std::string appInstanceId){appInstanceId_ = appInstanceId;};
    void setCommInterface(std::vector<SockAddr> commInterface){commInterface_ = commInterface;};

    std::string getAppInstanceId() const {return appInstanceId_;};
    std::vector<SockAddr> getCommInterface() const{ return commInterface_;};
};

#endif /* NODES_MEC_MECPLATFORM_MECSERVICES_APPLICATIONMOBILITYSERVICE_RESOURCES_TARGETINFO_H_ */
