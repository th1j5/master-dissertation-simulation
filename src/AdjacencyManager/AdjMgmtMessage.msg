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

import inet.common.INETDefs;
import inet.common.packet.chunk.Chunk;
import inet.networklayer.common.L3Address;
import inet.networklayer.contract.ipv4.Ipv4Address;
import inet.linklayer.common.MacAddress;

//
// Adjacency Managment Message
//

enum AdjMgmtOpcode
{
    LOCREQUEST = 1;
    LOCREPLY = 2;
}

class AdjMgmtMessage extends inet::FieldsChunk {
    AdjMgmtOpcode op;
    int seqNumber;
    inet::MacAddress sID; // server ID (MAC)
    inet::MacAddress cID; // client hardware address (MAC)
    inet::L3Address assignedLoc;
    inet::L3Address sLoc; // server Loc (= gateway)
    inet::Ipv4Address subnetMask; //needed because we use IPv4
}
