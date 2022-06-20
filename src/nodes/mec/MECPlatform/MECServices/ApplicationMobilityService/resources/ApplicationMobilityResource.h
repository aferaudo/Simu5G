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
#include "TargetAppInfo.h"

using namespace omnetpp;


class ApplicationMobilityResource : public AttributeBase{
  private:
    std::map<std::string, RegistrationInfo*> serviceConsumers_;

    // Apps for which migration has been successfully completed
    // key = appInstanceId
    // value = targetAppInfo where app is migrating
    std::map<std::string, TargetAppInfo*> migratedApps_;

    // Apps in migration phase - apps in this map are going to be migrated in the target ip address
    // key = appInstanceId
    // value = targetAppInfo
    std::map<std::string, TargetAppInfo*> migratingApps_;
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

    // Adds information about the new app correctly migrated
    bool addMigratedApp(TargetAppInfo *);

    // Removes app for which the migration phase is completed
    bool removingMigratedApp(std::string);

    // Add information about an app in migration phase
    // This method is called by the AMS for each subscriber
    // so it needs to check whether the corresponding app has already been recorded
    void addMigratingApp(TargetAppInfo *);

    // Removes app in migration phase
    // usually called by the AMS when the app has been successufully migrated
    bool removeMigratingApp(std::string);

    std::vector<std::string> getAppInstanceIds(std::vector<AssociateId> associateId);

    std::map<std::string, TargetAppInfo *> getMigratedApps() const {return migratedApps_;}

    // This method returns a registration info pointer from appInstanceId
    RegistrationInfo *getRegistrationInfoFromAppId(std::string appInstanceId) const;

    RegistrationInfo *getRegistrationInfoFromContext(std::string appInstanceId, ContextTransferState context=USER_CONTEXT_TRANSFER_COMPLETED) const;

};

#endif /* _APPLICATIONMOBILITYRESOURCE_H_ */
