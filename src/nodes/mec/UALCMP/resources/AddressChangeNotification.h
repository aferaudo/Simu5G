//
// 
// Authors:
// Angelo Feraudo
// 

#ifndef NODES_MEC_UALCMP_RESOURCES_ADDRESSCHANGENOTIFICATION_H_
#define NODES_MEC_UALCMP_RESOURCES_ADDRESSCHANGENOTIFICATION_H_

#include "UALCMPNotificationBase.h"

class AddressChangeNotification : public UALCMPNotificationBase{
    std::string appInstanceId;
    std::string referenceURI;

public:
    AddressChangeNotification();
    virtual ~AddressChangeNotification();

    virtual nlohmann::ordered_json toJson() const;
    virtual bool fromJson(const nlohmann::ordered_json& json);

    const std::string& getAppInstanceId() const {return appInstanceId;}

    void setAppInstanceId(const std::string &aID) {appInstanceId = aID;}

    const std::string& getReferenceUri() const {return referenceURI;}

    void setReferenceUri(const std::string &referenceUri) {referenceURI = referenceUri;}
};

#endif /* NODES_MEC_UALCMP_RESOURCES_ADDRESSCHANGENOTIFICATION_H_ */
