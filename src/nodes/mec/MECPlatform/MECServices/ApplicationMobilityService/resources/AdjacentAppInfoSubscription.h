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

#ifndef _ADJACENTAPPINFOSUBSCRIPTION_H_
#define _ADJACENTAPPINFOSUBSCRIPTION_H_
#include <omnetpp.h>
#include "nodes/mec/MECPlatform/MECServices/Resources/SubscriptionBase.h"

using namespace omnetpp;
// This subscription type has not been implemented yet

class AdjacentAppInfoSubscription : public SubscriptionBase{
  public:
    AdjacentAppInfoSubscription();
    virtual ~AdjacentAppInfoSubscription();

    virtual void sendSubscriptionResponse() {EV << "To be implemented" << endl;};
    virtual void sendNotification(EventNotification *event) {EV << "To be implemented" << endl;};
    virtual EventNotification* handleSubscription() {EV << "To be implemented" << endl;};
};

#endif /* _ADJACENTAPPINFOSUBSCRIPTION_H_ */
