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

#include "RoundRobinScheduler.h"

int RoundRobinScheduler::scheduleRemoteResources(ResourceDescriptor &r)
{
    EV << "RoundRobinScheduler::findBestHostDyn [round robin] - Start" << endl;

    int besthost_key = -1;
    int min = INT_MAX;

    for(auto it : getHandledHosts()){
        int key = it.first;
        HostDescriptor descriptor = (it.second);

        if (it.first == vim_->getParentModule()->getId() && vim_->par("skipLocalResources")){
            //considers only remote resources
            continue;
        }

        bool available = r.ram < descriptor.totalAmount.ram - descriptor.usedAmount.ram - descriptor.reservedAmount.ram
                    && r.disk < descriptor.totalAmount.disk - descriptor.usedAmount.disk - descriptor.reservedAmount.disk
                    && r.cpu  < descriptor.totalAmount.cpu - descriptor.usedAmount.cpu - descriptor.reservedAmount.cpu;

        if(available && descriptor.numRunningApp < min){
            min = descriptor.numRunningApp;
            besthost_key = key;
        }
    }

    if(besthost_key == -1){
        EV << "RoundRobinScheduler::findBestHostDyn [round robin] - Best host not found!" << endl;
        return -1;
    }

    EV << "RoundRobinScheduler::findBestHostDyn [round robin] - Found " << besthost_key << endl;

    return besthost_key;
}
