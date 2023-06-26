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

import inet.common.packet.chunk.Chunk;
import inet.networklayer.common.L3Address;
//
// TODO generated message class
//
cplusplus {{
typedef std::list<inet::L3Address> RoutingPath;
}}

enum MessageType {
    SOCIALANNOUNCE = 1;
    // should we do it this way? see Message::Type:: in U-Sphere
}
class RoutingPath {
    @existingClass;
    // Would be needed to silence warning, only import for the descriptor class
    // https://doc.omnetpp.org/omnetpp/manual/#sec:msg-defs:descriptor-classes
//    @toValue(.str());  // use IPAddress::str() to produce a string
//    @fromValue(IPAddress($));  // use constructor; '$' will be replaced by the string
}

class PathAnnounce extends inet::FieldsChunk {
    inet::L3Address origin;			// Originator ID (public key in U-Sphere)
    RoutingPath forward_path @byValue;	// Forward path
    RoutingPath reverse_path @byValue;	// Reverse path (included only for landmark announces)
    bool landmark;					// Landmark status of the origin
    uint32 seqno;					// Sequence number
    // Cryptographic path delegation chain
    //repeated bytes delegation_chain = 6;
}
