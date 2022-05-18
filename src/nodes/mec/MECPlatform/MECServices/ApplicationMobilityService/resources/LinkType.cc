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

#include "LinkType.h"

LinkType::LinkType() {
    // TODO Auto-generated constructor stub

}

LinkType::LinkType(std::string href)
{
    href_ = href;
}

LinkType::~LinkType() {
    // TODO Auto-generated destructor stub
}

nlohmann::ordered_json LinkType::toJson() const
{
    nlohmann::ordered_json val;
    val["href"] = href_;

    return val;
}

