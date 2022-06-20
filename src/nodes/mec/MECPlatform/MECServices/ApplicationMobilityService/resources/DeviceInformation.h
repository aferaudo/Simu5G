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

#ifndef NODES_MEC_MECPLATFORM_MECSERVICES_APPLICATIONMOBILITYSERVICE_RESOURCES_DEVICEINFORMATION_H_
#define NODES_MEC_MECPLATFORM_MECSERVICES_APPLICATIONMOBILITYSERVICE_RESOURCES_DEVICEINFORMATION_H_
#include <omnetpp.h>
#include "nodes/mec/MECPlatform/MECServices/Resources/AttributeBase.h"
#include "nodes/mec/MECPlatform/MECServices/RNIService/resources/AssociateId.h"

using namespace omnetpp;

enum AppMobilityServiceLevel {APP_MOBILITY_NOT_ALLOWED = 0, APP_MOBILITY_WITH_CONFIRMATION, APP_MOBILITY_WITHOUT_CONFIRMATION};
static const char* AppMobilityServiceLevelStrings [] = {"APP_MOBILITY_NOT_ALLOWED", "APP_MOBILITY_WITH_CONFIRMATION", "APP_MOBILITY_WITHOUT_CONFIRMATION"};
enum ContextTransferState {NOT_TRANSFERRED = 0, USER_CONTEXT_TRANSFER_COMPLETED};
static const char* ContextTransferStateStrings [] = {"NOT_TRANSFERRED", "USER_CONTEXT_TRANSFER_COMPLETED"};

class DeviceInformation: public AttributeBase {
  private:
    AssociateId associateId_;
    AppMobilityServiceLevel appMobilityServiceLevel_;
    ContextTransferState contextTransferState_;

  public:
    DeviceInformation();
    virtual ~DeviceInformation();
    virtual nlohmann::ordered_json toJson() const override;
    bool fromJson(const nlohmann::ordered_json& object);

    // setter methods
    void setAssociateId(AssociateId associateId){associateId_ = associateId;};
    void setAppMobilityServiceLevel(AppMobilityServiceLevel appMobilityServiceLevel){appMobilityServiceLevel_ = appMobilityServiceLevel;};
    void setAppMobilityServiceLevelFromString(std::string appMobilityServiceLevel);
    void setContextTransferState(ContextTransferState contextTransferState){contextTransferState_ = contextTransferState;};
    void setContextTransferStateFromString(std::string contextTransferState);

    // getter methods
    AssociateId getAssociateId() const {return associateId_;}
    AppMobilityServiceLevel getAppMobilityServiceLevel() const {return appMobilityServiceLevel_;}
    ContextTransferState getContextTransferState() const {return contextTransferState_;}
    std::string getAppMobilityServiceLevelString() const {return AppMobilityServiceLevelStrings[appMobilityServiceLevel_];}
    std::string getContextTransferStateString() const {return ContextTransferStateStrings[contextTransferState_];}

};

#endif /* NODES_MEC_MECPLATFORM_MECSERVICES_APPLICATIONMOBILITYSERVICE_RESOURCES_DEVICEINFORMATION_H_ */
