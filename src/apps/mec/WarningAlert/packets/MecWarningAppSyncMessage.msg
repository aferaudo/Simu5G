//
//                  Simu5G
//
// Authors: Giovanni Nardini, Giovanni Stea, Antonio Virdis (University of Pisa)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//


//
// IceAlert Packet used by UEIceAlertApp MEIceAlertApp and MEIceAlertService
//

import inet.common.INETDefs;
import inet.common.packet.chunk.Chunk;
import inet.networklayer.common.L3Address;

class MecWarningAppSyncMessage extends inet::FieldsChunk {
    int contextId;
    
    double positionX;
    double positionY;
    double radius;
    
    string state;
    
    inet::L3Address ueAddress;
    int uePort;
    
    
    bool isAck = false;
    bool result = false;
    
    
}


