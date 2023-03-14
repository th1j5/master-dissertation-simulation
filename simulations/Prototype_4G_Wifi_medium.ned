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

import Link;

//
// Generated network with random topology (80 nodes, 120 edges, seed=100).
//
network Prototype4GWifiMedium {
    parameters:
        int numUsers;
        @display("bgb=400,400;bgg=100,1,grey95");

        @figure[title](type=label; pos=0,-1; anchor=sw; color=darkblue);

        @figure[rcvdPkText](type=indicatorText; pos=310,20; anchor=w; font=,5; textFormat="packets received by user 0: %g"; initialValue=0);
        @statistic[packetReceived](source=users[0].app[0].packetReceived; record=figure(count); targetFigure=rcvdPkText);

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

        server: Router {
            parameters:
                @display("p=301,20;i=device/server2");
                // Hardcoded params
                //forwarding = false;
                hasOspf = true;
                hasUdp = true;
        }
        users[numUsers]: ManetRouter;
        node0 : Router;
        node1 : Router;
        node2 : Router;
        node3 : Router;
        node4 : Router;
        node5 : Router;
        node6 : Router;
        node7 : Router;
        node8 : Router;
        node9 : Router;
        node10 : Router;
        node11 : Router;
        node12 : Router;
        node13 : Router;
        node14 : Router;
        node15 : Router;
        node16 : Router;
        node17 : Router;
        node18 : Router;
        node19 : Router;
        node20 : Router;
        node21 : Router;
        node22 : Router;
        node23 : Router;
        node24 : Router;
        node25 : Router;
        node26 : Router;
        node27 : Router;
        node28 : Router;
        node29 : Router;
        node30 : Router;
        node31 : Router;
        node32 : Router;
        node33 : Router;
        node34 : Router;
        node35 : Router;
        node36 : Router;
        node37 : Router;
        node38 : Router;
        node39 : Router;
        node40 : Router;
        node41 : Router;
        node42 : Router;
        node43 : Router;
        node44 : Router;
        node45 : Router;
        node46 : Router;
        node47 : Router;
        node48 : Router;
        node49 : Router;
        node50 : Router;
        node51 : Router;
        node52 : Router;
        node53 : Router;
        node54 : Router;
        node55 : Router;
        node56 : Router;
        node57 : Router;
        node58 : Router;
        node59 : Router;
        node60 : Router;
        node61 : Router;
        node62 : Router;
        node63 : Router;
        node64 : Router;
        node65 : Router;
        node66 : Router;
        node67 : Router;
        node68 : Router;
        node69 : Router;
        node70 : Router;
        node71 : Router;
        node72 : Router;
        node73 : Router;
        node74 : Router;
        node75 : Router;
        node76 : Router;
        node77 : Router;
        node78 : Router;
        node79 : Router;
    connections:
        server.pppg++<--> Link <--> node20.pppg++; // Random node to connect to
        node0.pppg++ <--> Link <--> node1.pppg++;
        node0.pppg++ <--> Link <--> node50.pppg++;
        node0.pppg++ <--> Link <--> node67.pppg++;
        node0.pppg++ <--> Link <--> node19.pppg++;
        node0.pppg++ <--> Link <--> node21.pppg++;
        node0.pppg++ <--> Link <--> node6.pppg++;
        node0.pppg++ <--> Link <--> node22.pppg++;
        node0.pppg++ <--> Link <--> node11.pppg++;
        node0.pppg++ <--> Link <--> node75.pppg++;
        node1.pppg++ <--> Link <--> node16.pppg++;
        node1.pppg++ <--> Link <--> node2.pppg++;
        node1.pppg++ <--> Link <--> node3.pppg++;
        node1.pppg++ <--> Link <--> node19.pppg++;
        node1.pppg++ <--> Link <--> node5.pppg++;
        node1.pppg++ <--> Link <--> node62.pppg++;
        node2.pppg++ <--> Link <--> node4.pppg++;
        node2.pppg++ <--> Link <--> node8.pppg++;
        node2.pppg++ <--> Link <--> node30.pppg++;
        node3.pppg++ <--> Link <--> node65.pppg++;
        node3.pppg++ <--> Link <--> node7.pppg++;
        node3.pppg++ <--> Link <--> node10.pppg++;
        node4.pppg++ <--> Link <--> node54.pppg++;
        node4.pppg++ <--> Link <--> node24.pppg++;
        node4.pppg++ <--> Link <--> node27.pppg++;
        node5.pppg++ <--> Link <--> node33.pppg++;
        node5.pppg++ <--> Link <--> node12.pppg++;
        node5.pppg++ <--> Link <--> node14.pppg++;
        node6.pppg++ <--> Link <--> node18.pppg++;
        node6.pppg++ <--> Link <--> node66.pppg++;
        node6.pppg++ <--> Link <--> node51.pppg++;
        node6.pppg++ <--> Link <--> node41.pppg++;
        node6.pppg++ <--> Link <--> node13.pppg++;
        node6.pppg++ <--> Link <--> node47.pppg++;
        node7.pppg++ <--> Link <--> node17.pppg++;
        node7.pppg++ <--> Link <--> node34.pppg++;
        node7.pppg++ <--> Link <--> node39.pppg++;
        node7.pppg++ <--> Link <--> node9.pppg++;
        node7.pppg++ <--> Link <--> node26.pppg++;
        node7.pppg++ <--> Link <--> node74.pppg++;
        node8.pppg++ <--> Link <--> node59.pppg++;
        node8.pppg++ <--> Link <--> node28.pppg++;
        node9.pppg++ <--> Link <--> node48.pppg++;
        node9.pppg++ <--> Link <--> node35.pppg++;
        node9.pppg++ <--> Link <--> node37.pppg++;
        node12.pppg++ <--> Link <--> node52.pppg++;
        node12.pppg++ <--> Link <--> node56.pppg++;
        node12.pppg++ <--> Link <--> node15.pppg++;
        node13.pppg++ <--> Link <--> node69.pppg++;
        node14.pppg++ <--> Link <--> node49.pppg++;
        node14.pppg++ <--> Link <--> node52.pppg++;
        node14.pppg++ <--> Link <--> node38.pppg++;
        node14.pppg++ <--> Link <--> node57.pppg++;
        node14.pppg++ <--> Link <--> node63.pppg++;
        node15.pppg++ <--> Link <--> node20.pppg++;
        node15.pppg++ <--> Link <--> node68.pppg++;
        node15.pppg++ <--> Link <--> node74.pppg++;
        node16.pppg++ <--> Link <--> node79.pppg++;
        node17.pppg++ <--> Link <--> node48.pppg++;
        node17.pppg++ <--> Link <--> node35.pppg++;
        node17.pppg++ <--> Link <--> node25.pppg++;
        node18.pppg++ <--> Link <--> node19.pppg++;
        node19.pppg++ <--> Link <--> node32.pppg++;
        node19.pppg++ <--> Link <--> node20.pppg++;
        node19.pppg++ <--> Link <--> node61.pppg++;
        node19.pppg++ <--> Link <--> node30.pppg++;
        node20.pppg++ <--> Link <--> node29.pppg++;
        node21.pppg++ <--> Link <--> node70.pppg++;
        node22.pppg++ <--> Link <--> node23.pppg++;
        node23.pppg++ <--> Link <--> node41.pppg++;
        node23.pppg++ <--> Link <--> node57.pppg++;
        node23.pppg++ <--> Link <--> node47.pppg++;
        node24.pppg++ <--> Link <--> node76.pppg++;
        node24.pppg++ <--> Link <--> node46.pppg++;
        node24.pppg++ <--> Link <--> node47.pppg++;
        node25.pppg++ <--> Link <--> node71.pppg++;
        node26.pppg++ <--> Link <--> node64.pppg++;
        node26.pppg++ <--> Link <--> node36.pppg++;
        node26.pppg++ <--> Link <--> node73.pppg++;
        node26.pppg++ <--> Link <--> node31.pppg++;
        node27.pppg++ <--> Link <--> node68.pppg++;
        node27.pppg++ <--> Link <--> node54.pppg++;
        node27.pppg++ <--> Link <--> node29.pppg++;
        node28.pppg++ <--> Link <--> node53.pppg++;
        node28.pppg++ <--> Link <--> node60.pppg++;
        node28.pppg++ <--> Link <--> node30.pppg++;
        node29.pppg++ <--> Link <--> node68.pppg++;
        node29.pppg++ <--> Link <--> node40.pppg++;
        node30.pppg++ <--> Link <--> node64.pppg++;
        node30.pppg++ <--> Link <--> node42.pppg++;
        node31.pppg++ <--> Link <--> node45.pppg++;
        node31.pppg++ <--> Link <--> node61.pppg++;
        node32.pppg++ <--> Link <--> node52.pppg++;
        node32.pppg++ <--> Link <--> node72.pppg++;
        node33.pppg++ <--> Link <--> node56.pppg++;
        node33.pppg++ <--> Link <--> node74.pppg++;
        node33.pppg++ <--> Link <--> node77.pppg++;
        node34.pppg++ <--> Link <--> node43.pppg++;
        node35.pppg++ <--> Link <--> node39.pppg++;
        node37.pppg++ <--> Link <--> node56.pppg++;
        node38.pppg++ <--> Link <--> node78.pppg++;
        node39.pppg++ <--> Link <--> node55.pppg++;
        node39.pppg++ <--> Link <--> node44.pppg++;
        node41.pppg++ <--> Link <--> node51.pppg++;
        node41.pppg++ <--> Link <--> node74.pppg++;
        node41.pppg++ <--> Link <--> node75.pppg++;
        node41.pppg++ <--> Link <--> node44.pppg++;
        node43.pppg++ <--> Link <--> node76.pppg++;
        node44.pppg++ <--> Link <--> node64.pppg++;
        node44.pppg++ <--> Link <--> node48.pppg++;
        node47.pppg++ <--> Link <--> node48.pppg++;
        node47.pppg++ <--> Link <--> node76.pppg++;
        node48.pppg++ <--> Link <--> node58.pppg++;
        node52.pppg++ <--> Link <--> node74.pppg++;
        node58.pppg++ <--> Link <--> node63.pppg++;
        node61.pppg++ <--> Link <--> node69.pppg++;
        node63.pppg++ <--> Link <--> node73.pppg++;
        node67.pppg++ <--> Link <--> node79.pppg++;
        node72.pppg++ <--> Link <--> node74.pppg++;
        node73.pppg++ <--> Link <--> node79.pppg++;
        node74.pppg++ <--> Link <--> node75.pppg++;
}
