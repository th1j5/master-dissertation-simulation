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

package prototype.AdjacencyManager;

//
// Decides when to connect to neighbours.
//
simple AdjacencyManager
{
    parameters:
        @display("i=block/network2_l");
        @class(AdjacencyManager);
        @signal[newNeighConnected](type=cModule);
        @signal[oldNeighDisconnected](type=cModule);
        @statistic[newNeighConnected](title="new neigh link"; record=vector(nodeID); interpolationmode=none;);
        @statistic[oldNeighDisconnected](title="old neigh "; record=vector(nodeID); interpolationmode=none;);
        string interfaceTableModule; // path to the InterfaceTable module
        string routingTableModule; // path to the Ipv4RoutingTable module
}
