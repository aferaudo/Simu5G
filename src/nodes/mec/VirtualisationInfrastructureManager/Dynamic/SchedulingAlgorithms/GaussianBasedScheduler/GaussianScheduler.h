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

#ifndef NODES_MEC_VIRTUALISATIONINFRASTRUCTUREMANAGER_DYNAMIC_SCHEDULINGALGORITHMS_GAUSSIANBASEDSCHEDULER_GAUSSIANSCHEDULER_H_
#define NODES_MEC_VIRTUALISATIONINFRASTRUCTUREMANAGER_DYNAMIC_SCHEDULINGALGORITHMS_GAUSSIANBASEDSCHEDULER_GAUSSIANSCHEDULER_H_

#include "nodes/mec/VirtualisationInfrastructureManager/Dynamic/SchedulingAlgorithms/SchedulingAlgorithmBase.h"
#include <map>
#include <omnetpp.h>


struct GaussianDistr
{
    double mean;
    double std;
};


class GaussianScheduler : public SchedulingAlgorithmBase{

    std::string fileName_;
    std::map<int, GaussianDistr> gaussians_;
    float sampledTime_; // interval of data sampling (default 600s)

public:
    GaussianScheduler(VirtualisationInfrastructureManagerDyn *vim);
    virtual ~GaussianScheduler();
protected:
    virtual int scheduleRemoteResources(ResourceDescriptor &);
private:
    void loadingGaussianDistributions();
    int findKeyMaxOccupancyTime(std::map<int, HostDescriptor> subset);
};

#endif /* NODES_MEC_VIRTUALISATIONINFRASTRUCTUREMANAGER_DYNAMIC_SCHEDULINGALGORITHMS_GAUSSIANBASEDSCHEDULER_GAUSSIANSCHEDULER_H_ */
