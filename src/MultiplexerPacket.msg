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
import inet.applications.base.ApplicationPacket;
import inet.networklayer.common.L3Address;

class MultiplexerPacket extends inet::ApplicationPacket {
    // TODO: insert usefull info
    string multiplexerDestination;
    // client ID + seq number of loc Update, to correlate all update events
    // If in LocatorUpdatePacket, it is the corrID from the sender
    // If in MultiplexerPacket, it is the corrID known at that moment, from the receiver
    double locUpdateCorrelationID = -1;
}