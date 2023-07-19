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

package prototype.LocNodes;

import inet.node.inet.*;
import inet.applications.contract.IApp;

import prototype.AdjacencyManager.*;
import prototype.UniSphere.*;
import prototype.ReqRspLocAssignAlgo.*;

//
// TODO auto-generated module
//
module ScalableRouter extends Router {
    parameters:
        *.routingTableModule = default("^.generic.routingTable");
//        string routingTableModule = default(absPath("^.generic.routingTable"));
//        *.routingTableModule = this.routingTableModule;
    submodules:
        adjacencyManager: AdjacencyManager {
            parameters:
                @display("p=125,320");
        }
        unisphere: UniSphereControlPlane if exists(generic) { // might need an interface: "like IOspf" in inet.routing.contract
            parameters:
                @display("p=975,226");
        }
        hierLocAssignAlgo: HierarchicalLocAssignAlgo if exists(ipv4) {
            parameters:
                @display("p=1000,226");
        }
    connections:
        unisphere.networkLayerOut --> tn.in++ if exists(generic);
        unisphere.networkLayerIn <-- tn.out++ if exists(generic);
        hierLocAssignAlgo.networkLayerOut --> tn.in++ if exists(ipv4);
        hierLocAssignAlgo.networkLayerIn <-- tn.out++ if exists(ipv4);
}
module MobileScalableRouter extends ManetRouter {
    parameters:
        *.routingTableModule = default("^.generic.routingTable");
//        string routingTableModule = default(absPath("^.generic.routingTable"));
//        *.routingTableModule = this.routingTableModule;

        forwarding = false;
        // *.forwarding = false; // doesn't work
        //**.forwarding = false; // FIXME: doesn't work
    submodules:
        adjacencyManager: AdjacencyManager {
            parameters:
                @display("p=125,320");
        }
        unisphere: UniSphereControlPlane if exists(generic) { // might need an interface: "like IOspf" in inet.routing.contract
            parameters:
                @display("p=975,226");
        }
        hierLocAssignAlgo: HierarchicalLocAssignAlgo if exists(ipv4) {
            parameters:
                @display("p=1000,226");
        }
    connections:
        unisphere.networkLayerOut --> tn.in++ if exists(generic);
        unisphere.networkLayerIn <-- tn.out++ if exists(generic);
        hierLocAssignAlgo.networkLayerOut --> tn.in++ if exists(ipv4);
        hierLocAssignAlgo.networkLayerIn <-- tn.out++ if exists(ipv4);
}

// Final class nodes
module NeighbourNode extends ScalableRouter {
    parameters:
        *.adjacencyMgmt = default(absPath(".adjacencyManager"));
        numWlanInterfaces = default(1);
        **.radio.displayCommunicationRange = default(true);
        *.newLocInterface = "wlan0";
        // server/responder?
        hierLocAssignAlgo.server = true;
}
module RoutingNode extends ScalableRouter
{
    parameters:
        *.adjacencyMgmt = default(absPath(".adjacencyManager"));
}
// Can get new Loc (doesn't HAS to mean that it moves)
module MobileNode extends MobileScalableRouter {
    parameters:
        *.adjacencyMgmt = default(absPath(".adjacencyManager"));
        app[*].typename = default("UdpBasicConnectionApp");
        app[*].packetName = default("UDPData");    // don't change this, see Resultfilters
        app[*].newLocInterface = "wlan0"; // from which Interface to pick up Loc changes
        app[*].oldLocInterface = "wlan1"; // pick up unreachability of old Loc
        numWlanInterfaces = default(2);   // newLoc (wlan[0]) and oldLoc (wlan[1])
        *.newLocInterface = "wlan0";
        *.oldLocInterface = "wlan1";
        // client/requestor?
        hierLocAssignAlgo.client = true;
}
