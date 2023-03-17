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

#include "GaussianScheduler.h"
#include <fstream>
#include <sstream>

using namespace omnetpp;

GaussianScheduler::GaussianScheduler(VirtualisationInfrastructureManagerDyn *vim):SchedulingAlgorithmBase(vim)
{
    fileName_ = "OccTimeGaussianDistr10Minutes.csv";
    loadingGaussianDistributions();
    sampledTime_ = 600;

}

GaussianScheduler::~GaussianScheduler()
{
}

void GaussianScheduler::loadingGaussianDistributions()
{
    EV << "GaussianScheduler::LOADING Gaussian distributions from " << fileName_ << endl;
    std::string line, word;
    char sep = ';';
    GaussianDistr *currentGauss = nullptr;
    int currentTime;

    std::fstream file (fileName_, std::ios::in);
    int k, i = 0;
    if(file.is_open())
    {
        while(getline(file, line))
		{
            if(i != 0) // Skipping the first line containing the header
            {
                std::stringstream str(line);
                k = 0;
                currentGauss = new GaussianDistr();
               
			    while(getline(str, word, sep))
                {
                    switch(k)
                    {
                        case 2:
                        {
                            currentGauss->mean = std::stof(word);
                            break;
                        }
                        case 3:
                        {
                            currentGauss->std = std::stof(word);
                            break;
                        }
                        case 4:
                        {
                            currentTime = std::stoi(word);
                            break;
                        }
                        default:
                        {
                            break;
                        }
                    }
                    k ++;

                }

                if(k > 3)
                {
                    gaussians_[currentTime] = (*currentGauss);
                }

            }
            i++;
        }
    }
    else
        throw cRuntimeError("GaussianScheduler::Could not load csv file!");



//    cout << "Printing file content" << endl;
//    for (auto itr = gaussians_.begin(); itr != gaussians_.end(); ++itr) {
//        cout << itr->first
//             << '\t' << itr->second.mean << '\t' << itr->second.std << endl;
//    }
    
    file.close();
    EV << "GaussianScheduler::LOEADED Gaussian distributions" << endl;
}

int GaussianScheduler::findKeyMaxOccupancyTime(std::map<int, HostDescriptor> subset)
{
    float max = 0;
    int maxKey = -1;
    EV << "GaussianScheduler::finding max key" << endl;
    for(auto remoteHost : subset)
    {
        if(remoteHost.second.predictedOccupancyTime > max)
        {
            max = remoteHost.second.predictedOccupancyTime;
            maxKey = remoteHost.first;
        }
    }

    return maxKey;
}

int GaussianScheduler::scheduleRemoteResources(ResourceDescriptor &r)
{
    EV << "GaussianScheduler::scheduleRemoteResources started" << endl;
    
    std::map<int, HostDescriptor> valuableHosts;
    int key = -1;
    int min = INT_MAX;
    int maxOccupancyTime = 0;
    int occupancyTimeIndex;


    for(auto remoteHost : getHandledHosts())
    {

        //verify that the hosts has enough resources
        bool available = r.ram < remoteHost.second.totalAmount.ram - remoteHost.second.usedAmount.ram - remoteHost.second.reservedAmount.ram
                            && r.disk < remoteHost.second.totalAmount.disk - remoteHost.second.usedAmount.disk - remoteHost.second.reservedAmount.disk
                            && r.cpu  < remoteHost.second.totalAmount.cpu - remoteHost.second.usedAmount.cpu - remoteHost.second.reservedAmount.cpu;

        if(available && remoteHost.second.numRunningApp < min)
        {
            //Looking for occupancy time
            double occupancyTime = remoteHost.second.predictedOccupancyTime;
            if(occupancyTime == -1)
            {
                //computing occupancy time

                //finding index
                float test = std::fmod(remoteHost.second.entranceTime,sampledTime_);
                if((test/sampledTime_) < 0.5)
                {
                    occupancyTimeIndex = remoteHost.second.entranceTime - test;
                }
                else
                {
                    occupancyTimeIndex = remoteHost.second.entranceTime + test;
                }

//                occupancyTime = truncnormal((double)gaussians_[occupancyTimeIndex].mean, (double)gaussians_[occupancyTimeIndex].std) * 60; // from minutes to seconds
            }
        }
    }


    EV << "GaussianScheduler::scheduleRemoteResources ended" << endl;

    key = findKeyMaxOccupancyTime(valuableHosts);
    return key;
}

