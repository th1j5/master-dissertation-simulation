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

package prototype.ReqRspLocAssignAlgo;

import prototype.LocUpdatable.*;

//
// A Request Respons Locator Assignement Algorithm
// which asks it parent what address it should use.
//
simple HierarchicalLocAssignAlgo like ILocUpdatable {
    parameters:
        @display("i=block/table2_l");
        @class(HierarchicalLocAssignAlgo);
        @signal[neighLocatorUpdateSent](type=inet::Packet);
        @signal[oldLocatorUnreachable](type=int); // int numLocUpdates
        @signal[newLocatorAssigned](type=int); // int numLocUpdates
        @statistic[oldLocatorUnreachable](title="old locator becomes unreachable";record=vector(locUpdateCorrelation); interpolationmode=none;);
        @statistic[newLocatorAssigned](title="new locator assigned";record=vector(locUpdateCorrelation); interpolationmode=none;);
        string routingTableModule;
        string interfaceTableModule;
        bool client = default(false);
        bool server = default(false);
        // client
        string newLocInterface = default(""); // which NIC should be configured (i.e. wlan0, eth1 etc.)
        string oldLocInterface = default("");
        // server
        int maxNumClients = default(-1); // maximum number of clients (IPs) allowed to be leased

        // Should be part of general control plane, not this/here
        string locChangingStrategy = default("end2end"); // Which strategy to use for Loc changes
        string transientTriangularRoutingModule = default("^.generic.ttrTable");
    gates:
        input networkLayerIn;
        output networkLayerOut;
}
