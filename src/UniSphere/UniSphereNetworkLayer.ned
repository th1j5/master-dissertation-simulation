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
import inet.common.MessageDispatcher;
import inet.networklayer.arp.ipv4.GlobalArp;
import inet.networklayer.common.EchoProtocol;
import inet.networklayer.contract.INetworkLayer;

import prototype.LocUpdatable.*;

module UniSphereControlPlane like ILocUpdatable {
    parameters:
        @class(UniSphereControlPlane);
        @signal[isLandmark](type=bool);
        @signal[neighLocatorUpdateSent](type=inet::Packet);
        @signal[oldLocatorUnreachable](type=int); // int numLocUpdates
        @signal[newLocatorAssigned](type=int); // int numLocUpdates
        @statistic[oldLocatorUnreachable](title="old locator becomes unreachable";record=vector(locUpdateCorrelation); interpolationmode=none;);
        @statistic[newLocatorAssigned](title="new locator assigned";record=vector(locUpdateCorrelation); interpolationmode=none;);
        @statistic[isLandmark](title="landmark?";record=last;);
        string routingTableModule;
        string interfaceTableModule;
        bool forwarding = default(true);

        // Should be part of general control plane, not this/here
        string locChangingStrategy = default("end2end"); // Which strategy to use for Loc changes
    gates:
        input networkLayerIn;
        output networkLayerOut;
}
module UniSphereForwarding extends NextHopForwarding {
    parameters:
        @class(UniSphereForwarding);
}
// NextHopNetworkLayer was not extendable
module UniSphereDataPlane like INetworkLayer {
    parameters:
//        @class();
        //string locChangingStrategy;

        bool forwarding = default(false);
        bool multicastForwarding = default(false);
        string interfaceTableModule;
        *.forwarding = this.forwarding;
        *.multicastForwarding = this.multicastForwarding;
        *.interfaceTableModule = default(absPath(this.interfaceTableModule));
        *.routingTableModule = default(absPath(".routingTable"));
        *.arpModule = default(absPath(".arp"));
        @display("i=block/fork");
    gates:
        input ifIn @labels(INetworkHeader);
        output ifOut @labels(INetworkHeader);
        input transportIn @labels(Ipv4ControlInfo/down);
        output transportOut @labels(Ipv4ControlInfo/up);

    submodules:
        routingTable: NextHopRoutingTable {
            parameters:
                @display("p=100,100;is=s");
        }
        echo: EchoProtocol {
            parameters:
                @display("p=600,100");
        }
        up: MessageDispatcher {
            parameters:
                @display("p=450,200;b=480,5");
        }
        arp: GlobalArp {
            parameters:
                @display("p=300,300;q=pendingQueue");
        }
        nextHop: UniSphereForwarding {
            parameters:
                @display("p=600,300;q=queue");
        }
        lp: MessageDispatcher {
            parameters:
                @display("p=450,400;b=480,5");
        }
        //ttrTable: Ttr if locChangingStrategy == "TTR" {
        //    @display("p=100,400;is=s");
        //}
//        routingTable: UniSphereRoutingTable {
//            parameters:
//                @display("p=100,200;is=s");
//        }

    connections allowunconnected:
        up.out++ --> { @display("m=n"); } --> transportOut;
        up.in++ <-- { @display("m=n"); } <-- transportIn;

        nextHop.transportOut --> up.in++;
        nextHop.transportIn <-- up.out++;

        up.out++ --> echo.ipIn;
        up.in++ <-- echo.ipOut;

        lp.out++ --> arp.ifIn;
        lp.in++ <-- arp.ifOut;

        lp.out++ --> nextHop.queueIn;
        lp.in++ <-- nextHop.queueOut;

        lp.in++ <-- { @display("m=s"); } <-- ifIn;
        lp.out++ --> { @display("m=s"); } --> ifOut;
}
module UniSphereRoutingTable {
    @class(UniSphereRoutingTable);
}
