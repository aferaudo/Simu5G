/*
 * Entries.h
 *
 *  Created on: Jun 20, 2023
 *      Author: Angelo Feraudo
 */

#ifndef APPS_MEC_RESOURCESHARINGAPPS_UTILS_ENTRIES_H_
#define APPS_MEC_RESOURCESHARINGAPPS_UTILS_ENTRIES_H_
#include "inet/common/geometry/common/Coord.h"


struct ClientResourceEntry
{
    int clientId;
    inet::L3Address ipAddress;
    int viPort; // used by the VIM to allocate new app
    ResourceDescriptor resources;
    std::string reward;
    inet::Coord coord;
    int vimId = -1;
    bool operator < (const ClientResourceEntry &other) const {return clientId < other.clientId;}
};

struct SubscriberEntry
{
    int clientId;
    // this parameters maybe useful in case of connection lost
    inet::L3Address clientAddress;
    int clientPort;
    // -----
    std::string clientWebHook;
    inet::Coord subscriberPos;
    double subscriberRadius;
    bool operator < (const SubscriberEntry &other) const {return clientId < other.clientId;}
};

#endif /* APPS_MEC_RESOURCESHARINGAPPS_UTILS_ENTRIES_H_ */
