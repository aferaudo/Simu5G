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

#ifndef NODES_MEC_MECPLATFORM_MECSERVICES_APPLICATIONMOBILITYSERVICE_RESOURCES_WEBSOCKNOTIFCONFIG_H_
#define NODES_MEC_MECPLATFORM_MECSERVICES_APPLICATIONMOBILITYSERVICE_RESOURCES_WEBSOCKNOTIFCONFIG_H_
#include<omnetpp.h>
#include "nodes/mec/MECPlatform/MECServices/Resources/AttributeBase.h"

using namespace omnetpp;

class WebsockNotifConfig: public AttributeBase {
  private:
    std::string websocketUri_;
    bool requestWebsocketUri_;

  public:
    WebsockNotifConfig();
    virtual ~WebsockNotifConfig();
    virtual nlohmann::ordered_json toJson() const;
    virtual bool fromJson(const nlohmann::ordered_json& object);

    void setWebSocketUri(std::string websocketUri) {websocketUri_ = websocketUri;};
    void setRequestWebsocketUri(bool requestWebsocketUri) {requestWebsocketUri_ = requestWebsocketUri;};

    std::string getWebSocketUri() const {return websocketUri_;};
    bool getRequestWebSocketUri() const {return requestWebsocketUri_;};
};

#endif /* NODES_MEC_MECPLATFORM_MECSERVICES_APPLICATIONMOBILITYSERVICE_RESOURCES_WEBSOCKNOTIFCONFIG_H_ */
