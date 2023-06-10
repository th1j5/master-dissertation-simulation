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

package prototype.UniSphere;

import inet.networklayer.nexthop.*;

module UniSphereControlPlane {
    @class(UniSphereControlPlane);
    gates:
        input networkLayerIn;
        output networkLayerOut;
}
module UniSphereDataPlane extends NextHopNetworkLayer {
    parameters:
//        @class();
        //string locChangingStrategy;
    submodules:
        //ttrTable: Ttr if locChangingStrategy == "TTR" {
        //    @display("p=100,400;is=s");
        //}
//        routingTable: UniSphereRoutingTable {
//            parameters:
//                @display("p=100,200;is=s");
//        }
}
module UniSphereRoutingTable {
    @class(UniSphereRoutingTable);
}
