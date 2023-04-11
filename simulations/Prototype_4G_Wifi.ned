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

package prototype.simulations;

import inet.node.inet.*;
import inet.node.ethernet.*;
import inet.networklayer.configurator.ipv4.*;
import inet.networklayer.configurator.nexthop.*;
import inet.physicallayer.wireless.common.medium.RadioMedium;
import inet.physicallayer.wireless.unitdisk.*;
import inet.visualizer.contract.IIntegratedVisualizer;

import prototype.TTR.TtrIpv4NetworkLayer;
import prototype.DelayLink;
import prototype.LocNodes.*;

network Prototype4GWifi
{
    parameters:
        **.ipv4.typename = "TtrIpv4NetworkLayer";
        int numUsers;
        int numRoutersAlongPath;
        @display("bgb=400,400;bgg=100,1,grey95");

        @figure[title](type=label; pos=0,-1; anchor=sw; color=darkblue);

        @figure[rcvdPkText](type=indicatorText; pos=310,10; anchor=w; font=,5; textFormat="packets received by user 0: %g"; initialValue=0);
        @figure[sentPkText](type=indicatorText; pos=310,20; anchor=w; font=,5; textFormat="packets send to user 0: %g"; initialValue=0);
        @figure[sentLocUpdText](type=indicatorText; pos=310,30; anchor=w; font=,5; textFormat="Loc updates send from user 0 to server: %g"; initialValue=0);
        @figure[rcvdLocUpdText](type=indicatorText; pos=310,40; anchor=w; font=,5; textFormat="Loc updates received by server (by user?): %g"; initialValue=0);
        @statistic[packetReceived](source=users[0].app[0].packetReceived; record=figure(count); targetFigure=rcvdPkText);
        @statistic[packetSent](source=server.app[0].packetSent; record=figure(count); targetFigure=sentPkText);
        @statistic[locUpdSent](source=users[0].app[0].locatorUpdateSent; record=figure(count); targetFigure=sentLocUpdText);
        @statistic[locUpdRcvd](source=server.app[0].locatorUpdateReceived; record=figure(count); targetFigure=rcvdLocUpdText);
    submodules:
        visualizer: <default(firstAvailableOrEmpty("IntegratedCanvasVisualizer"))> like IIntegratedVisualizer if typename != "" {
            @display("p=15,35");
        }
        configurator: Ipv4NetworkConfigurator {
            @display("p=15,15");
        }
        radioMedium: UnitDiskRadioMedium {
            @display("p=15,55");
        }

        server: RoutingNode {
            parameters:
                @display("p=301,20;i=device/server2");
                // Hardcoded params
                //forwarding = false;
                hasOspf = true;
                hasUdp = true;
        }
        serverGateway: RoutingNode;
        routersAlongPath[numRoutersAlongPath]: RoutingNode {
            parameters:
                // FIXME: packetDropped
                @statistic[endToEndDelayLost](title="end-to-end delay lost packets"; source="dataAge(packetDropped)"; unit=s; record=histogram,vector; interpolationmode=none);
                @statistic[corrIDLost](title="correlation ID lost packets"; source="locUpdateCorrelation(udpData(packetDropped))"; record=vector; interpolationmode=none);
        }
        users[numUsers]: MobileNode;
    connections:
        server.pppg++ <--> DelayLink <--> serverGateway.pppg++;
        for i=0..numRoutersAlongPath-1 {
            serverGateway.pppg++ <--> DelayLink <--> routersAlongPath[i].pppg++;
        }
        // First connection
        // routersAlongTrack[0].pppg++ <--> {  datarate = 1Gbps; } <--> trainTransceiver.pppg++;
}
