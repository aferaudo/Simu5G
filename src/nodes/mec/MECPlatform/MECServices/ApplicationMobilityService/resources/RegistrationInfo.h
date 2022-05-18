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

#ifndef _REGISTRATIONINFO_H_
#define _REGISTRATIONINFO_H_

#include<omnetpp.h>
#include "nodes/mec/MECPlatform/MECServices/Resources/AttributeBase.h"
#include "nodes/mec/MECPlatform/MECServices/RNIService/resources/AssociateId.h"
#include "DeviceInformation.h"

using namespace omnetpp;



struct ServiceConsumerId
{
    std::string appInstanceId;
    std::string mepId;
};


class RegistrationInfo : public AttributeBase{
  private:
    std::string appMobilityServiceId;
    std::vector<DeviceInformation> deviceInformation;
    int expiryTime; // not mandatory
    ServiceConsumerId serviceConsumerId;

  public:
    RegistrationInfo();
    RegistrationInfo(ServiceConsumerId scId);
    RegistrationInfo(ServiceConsumerId scId, std::string appMobSId, std::vector<DeviceInformation>  devInfo, int exTime);
    virtual ~RegistrationInfo();

    virtual nlohmann::ordered_json toJson() const override;

    bool fromJson(const nlohmann::ordered_json& object);

    // set methods
    void setAppMobilityServiceId(std::string appMobSId);

    // get methods
    std::string getAppMobilityServiceId() const {return appMobilityServiceId;}
    std::vector<DeviceInformation> getDeviceInformation() const {return deviceInformation;}
    int getExpiryTime() const {return expiryTime;}
    ServiceConsumerId getServiceConsumerId() const {return serviceConsumerId;}

    void printRegistrationInfo();

};

#endif /* _REGISTRATIONINFO_H_ */
