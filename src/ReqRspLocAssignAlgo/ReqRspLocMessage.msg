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

import MultiplexerPacket;
import Locator;

//
// Adjacency Managment Message
//

enum ReqRspLocOpcode
{
    LOCREQUEST = 1;
    LOCREPLY = 2;
}

class ReqRspLocMessage extends MultiplexerPacket
{
    multiplexerDestination = "adj_mgmt";
    ReqRspLocOpcode op;
    int seqNumber;
    inet::L3Address   sID;         // server ID (MAC)
    inet::L3Address   cID;         // client hardware address (MAC)
    Locator           assignedLoc;

    // needed for routes & dataPlane purposeses, not clear how much is actually implemented in INET
    // i.e., maybe we could drop this info and always use a 32 length netmask
    inet::Ipv4Address subnetMask;  // needed because we use IPv4

    // PiggyBack info - only used for route assignement, NOT for locAssignment
    //inet::L3Address   sLoc;        // server Loc (= gateway)
    inet::Ipv4Address sIface;      // server Iface address (= gateway)
}
