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

#include "StateUtils.h"
#include "inet/common/packet/Packet.h"
#include "inet/common/TagBase_m.h"
#include "inet/common/TimeTag_m.h"
#include "inet/common/ProtocolTag_m.h"

#include "common/utils/utils.h"

#include <omnetpp.h>


namespace stateutils
{
    using namespace omnetpp;
    
    MsgState addStateChunk(std::string* data, MecAppSyncMessage* msg)
    {
        // std::cout << "Entering addStateChunk" << endl;
        int length = data->length();
        int remainingLength = msg->getRemainingDataToRecv();
        // debug
        // std::cout << "stateutils::addStateChunk: length: " << length << endl;
        // std::cout << "stateutils::addStateChunk: remainingLength: " << remainingLength << endl;

        if(remainingLength == 0)
        {
            EV << "stateutils::addStateChunk: remainingLength == 0" << endl;
            return COMPLETE_DATA;
        }

        EV << "stateutils::addStateChunk: length: " << length << endl;

        // Only for the state can exist something to receive
        std::string newState = std::string(msg->getState());
        newState = newState + data->substr(0, remainingLength);
        msg->setState(newState.c_str());

        data->erase(0, remainingLength);
        msg->setRemainingDataToRecv(remainingLength - (length - data->length()));
        // std::cout << "stateutils::addStateChunk: remainingLength after processing: " << msg->getRemainingDataToRecv() << endl;

        if(data->length() == 0 && msg->getRemainingDataToRecv() == 0)
        {
            return COMPLETE_DATA;
        }
        
        return INCOMPLETE_DATA;
    }

        
    MecAppSyncMessage* parseHeader(const std::string &data)
    {
        EV << "stateutils::parseHeader: parsing header" << endl;

        // debug
        // EV << "stateutils::parseHeader: data: " << data << endl;

        MecAppSyncMessage* msg = new MecAppSyncMessage();

        std::vector<std::string> header = simu5g::utils::splitString(data, "\r\n");
        
        msg->setContentLength(std::stoi(header[0]));
        msg->setRemainingDataToRecv(std::stoi(header[0]));
        msg->setContentType(header[1].c_str());
        msg->setIsAck(std::stoi(header[2]));
        msg->setResult(std::stoi(header[3]));
        msg->setArrivalTime(SimTime(std::stod(header[4])));

        return msg;

    }

    bool parseStateData(int sockId, std::string& packet, omnetpp::cQueue& messageQueue, MecAppSyncMessage** currentMsg)
    {
        EV_INFO << "stateutils::Parsing data start..." << endl;
        // state is separated from the rest of the message by this delimiter 
        // (two new lines look at getPayload function)
        std::string delimiter = "\r\n\r\n"; 
        size_t pos = 0;
        std::string header;
        bool completeMsg = false;
        if (*currentMsg != nullptr)
        {
            // Processing remaining body parts
            EV << "stateutils::parseStateData: processing remaining body parts" << endl;
            stateutils::MsgState state = addStateChunk(&packet, *currentMsg);

            switch(state)
            {
                case COMPLETE_DATA:
                    EV_INFO << "stateutils::COMPLETE_DATA" << endl;
                    completeMsg = true;
                    (*currentMsg)->setSockId(sockId);
                    messageQueue.insert(*currentMsg);
                    break;
                case INCOMPLETE_DATA:
                    EV_INFO << "stateutils::INCOMPLETE_DATA" << endl;
                    completeMsg = false;
                    break;
            }
        }

        // Processing header
        // npos is a static member constant value with the greatest 
        // possible value for an element of type size_t.
        while((pos=packet.find(delimiter)) != std::string::npos)
        {
            EV << "stateutils::parseStateData: delimiter found at: " << pos << endl;
            EV << "stateutils::processing header" << endl;
            header = packet.substr(0, pos);
            packet.erase(0, pos + delimiter.length());
            // Processing header
            *currentMsg = parseHeader(header);
            
            // Processing state
            stateutils::MsgState state = addStateChunk(&packet, *currentMsg);
            switch(state)
            {
                case COMPLETE_DATA:
                    EV_INFO << "stateutils::COMPLETE_DATA" << endl;
                    std::cout << "stateutils::parseStateData - complete data "<< endl;
                    completeMsg = true;
                    (*currentMsg)->setSockId(sockId);
                    messageQueue.insert(*currentMsg);
                    break;
                case INCOMPLETE_DATA:
                    EV_INFO << "stateutils::INCOMPLETE_DATA" << endl;
                    std::cout << "stateutils::parseStateData - incomplete data "<< endl;
                    completeMsg = false;
                    break;
            }
        }
        return completeMsg;
    }

    std::string getPayload(const inet::Ptr<const MecAppSyncMessage>& msg, int &dim)
    {
        std::string crlf = "\r\n";
        std::string payload = std::to_string(msg->getContentLength()) + crlf;
        payload += msg->getContentType() + crlf;
        payload += std::to_string(msg->isAck()) + crlf;
        payload += std::to_string(msg->getResult()) + crlf;
        payload += std::to_string(msg->getArrivalTime().dbl()) + crlf;
        dim = payload.length() + crlf.length();
        payload += crlf + std::string(msg->getState());

        return payload;
    }
}

