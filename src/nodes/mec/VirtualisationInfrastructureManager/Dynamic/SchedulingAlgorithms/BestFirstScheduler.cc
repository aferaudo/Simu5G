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

#include "BestFirstScheduler.h"

int BestFirstScheduler::scheduleRemoteResources(ResourceDescriptor &r)
{
    EV << "BestFirstScheduler::finding best remote host" << endl;
    for(auto it : getHandledHosts()){
        int key = it.first;
        HostDescriptor descriptor = (it.second);

        if (it.first == vim_->getParentModule()->getId()){
            // Skipping Local Resources always
            continue;
        }

        bool available = r.ram < descriptor.totalAmount.ram - descriptor.usedAmount.ram - descriptor.reservedAmount.ram
                    && r.disk < descriptor.totalAmount.disk - descriptor.usedAmount.disk - descriptor.reservedAmount.disk
                    && r.cpu  < descriptor.totalAmount.cpu - descriptor.usedAmount.cpu - descriptor.reservedAmount.cpu;

        if(available){
            EV << "VirtualisationInfrastructureManagerDyn::findBestHostDyn [best first] - Found " << key << endl;
            return key;
        }
    }

    EV << "BestFirstScheduler::finding best remote host failed!" << endl;
    return -1;
}
