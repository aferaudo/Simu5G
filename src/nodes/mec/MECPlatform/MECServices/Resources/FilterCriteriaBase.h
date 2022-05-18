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

#ifndef NODES_MEC_MECPLATFORM_MECSERVICES_RESOURCES_FILTERCRITERIABASE_H_
#define NODES_MEC_MECPLATFORM_MECSERVICES_RESOURCES_FILTERCRITERIABASE_H_
#include <omnetpp.h>
#include "AttributeBase.h"

using namespace omnetpp;

class FilterCriteriaBase : public AttributeBase{
  protected:
    std::string appInstanceId_;

  public:
    FilterCriteriaBase();
    virtual ~FilterCriteriaBase();
    virtual nlohmann::ordered_json toJson() const = 0;
    virtual bool fromJson(const nlohmann::ordered_json& json) = 0;

    void setAppInstanceId(std::string appInstanceId) {appInstanceId_ = appInstanceId;};
    std::string getAppInstanceId() const {return appInstanceId_;};
};

#endif /* NODES_MEC_MECPLATFORM_MECSERVICES_RESOURCES_FILTERCRITERIABASE_H_ */
