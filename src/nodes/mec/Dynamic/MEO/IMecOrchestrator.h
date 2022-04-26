/*
 * IMecOrchestrator.h
 *
 *  Created on: Mar 16, 2022
 *      Author: Alessandro Calvio, Angelo Feraudo
 */

#ifndef NODES_MEC_DYNAMIC_IMECORCHESTRATOR_H_
#define NODES_MEC_DYNAMIC_IMECORCHESTRATOR_H_

#include "nodes/mec/MECOrchestrator/ApplicationDescriptor/ApplicationDescriptor.h"


class IMecOrchestrator
{
    public:
        ~IMecOrchestrator() {};
        virtual const ApplicationDescriptor* getApplicationDescriptorByAppName(std::string& appName) const = 0;
        virtual const std::map<std::string, ApplicationDescriptor>* getApplicationDescriptors() const = 0;
};



#endif /* NODES_MEC_DYNAMIC_IMECORCHESTRATOR_H_ */
