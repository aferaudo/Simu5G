//
// Author:
// Alessandro Calvio
// Angelo Feraudo
// 

#ifndef _APPLICATIONMOBILITYRESOURCE_H_
#define _APPLICATIONMOBILITYRESOURCE_H_

#include <omnetpp.h>
#include "nodes/mec/MECPlatform/MECServices/Resources/AttributeBase.h"
#include "RegistrationInfo.h"

using namespace omnetpp;


class ApplicationMobilityResource : public AttributeBase{
  private:
    std::map<std::string, RegistrationInfo*> serviceConsumers_;

    void printRegisteredServiceConsumers();

  public:
    ApplicationMobilityResource();
    virtual ~ApplicationMobilityResource();

    virtual nlohmann::ordered_json toJson() const override;
    nlohmann::ordered_json toJsonFromId(std::string appMobilityServiceId);

    // method called after a post request
    void addRegistrationInfo(RegistrationInfo* r);

    // method called after a put request
    // Returns false if service consumer not found
    bool updateRegistrationInfo(std::string appMobilityServiceId, RegistrationInfo *r);

    // method called after a delete request
    // Returns false if service consumer not found
    bool removeRegistrationInfo(std::string appMobilityServiceID);

    std::vector<int> getAppInstanceIds(std::vector<AssociateId> associateId);
};

#endif /* _APPLICATIONMOBILITYRESOURCE_H_ */
