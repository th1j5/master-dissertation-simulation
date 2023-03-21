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

import inet.applications.contract.IApp;

//
// TODO auto-generated module
//
simple AdjacencyManager like IApp
{
    parameters:
        string interfaceTableModule; // path to the InterfaceTable module
//        string routingTableModule; // path to the Ipv4RoutingTable module
        string newLocInterface = default(""); // which NIC should be configured (i.e. wlan0, eth1 etc.)
        string oldLocInterface = default(""); // which NIC should be configured (i.e. wlan0, eth1 etc.)
        double startTime @unit(s) = default(0s); // starting time of DHCP initialization
        @display("i=block/app2");
//        @lifecycleSupport;
//        double stopOperationExtraTime @unit(s) = default(-1s);    // extra time after lifecycle stop operation finished
//        double stopOperationTimeout @unit(s) = default(2s);    // timeout value for lifecycle stop operation
//        @selfMessageKinds(inet::DhcpTimerType);
    gates:
        input socketIn @labels(UdpControlInfo/up);
        output socketOut @labels(UdpControlInfo/down);   
}
