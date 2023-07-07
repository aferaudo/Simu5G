/*
 * UALCMPNotificationBase.h
 *
 *  Created on: Jul 6, 2023
 *      Author: Angelo Feraudo
 */

#ifndef NODES_MEC_UALCMP_RESOURCES_UALCMPNOTIFICATIONBASE_H_
#define NODES_MEC_UALCMP_RESOURCES_UALCMPNOTIFICATIONBASE_H_

#include "nodes/mec/utils/httpUtils/json.hpp"


class UALCMPNotificationBase {
protected:
    std::string notificationType;
    std::string contextId;
public:
    UALCMPNotificationBase(){};
    ~UALCMPNotificationBase(){};

    virtual nlohmann::ordered_json toJson() const = 0;
    virtual bool fromJson(const nlohmann::ordered_json& json) = 0;

    virtual std::string getNotificationType() const {return notificationType;};
    virtual void setContextId(std::string cId) {contextId = cId;};
    virtual std::string getContextId() const {return contextId;};

};

#endif /* NODES_MEC_UALCMP_RESOURCES_UALCMPNOTIFICATIONBASE_H_ */
