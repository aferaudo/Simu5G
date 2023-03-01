/*
 * SchedulingAlgorithmBase.h
 *
 *  Created on: Mar 1, 2023
 *      Author: Angelo Feraudo
 */

#ifndef NODES_MEC_VIRTUALISATIONINFRASTRUCTUREMANAGER_DYNAMIC_SCHEDULINGALGORITHMS_SCHEDULINGALGORITHMBASE_H_
#define NODES_MEC_VIRTUALISATIONINFRASTRUCTUREMANAGER_DYNAMIC_SCHEDULINGALGORITHMS_SCHEDULINGALGORITHMBASE_H_

#include "nodes/mec/VirtualisationInfrastructureManager/Dynamic/VirtualisationInfrastructureManagerDyn.h"

class VirtualisationInfrastructureManagerDyn;

class SchedulingAlgorithmBase {

  friend class VirtualisationInfrastructureManagerDyn;

  protected:
      VirtualisationInfrastructureManagerDyn *vim_;
      virtual int scheduleRemoteResources(ResourceDescriptor &) = 0;
      std::map<int, HostDescriptor>& getHandledHosts() const {return vim_->handledHosts;}

  public:
    SchedulingAlgorithmBase(VirtualisationInfrastructureManagerDyn *vim){vim_=vim;}
    virtual ~SchedulingAlgorithmBase(){};
};

#endif /* NODES_MEC_VIRTUALISATIONINFRASTRUCTUREMANAGER_DYNAMIC_SCHEDULINGALGORITHMS_SCHEDULINGALGORITHMBASE_H_ */
