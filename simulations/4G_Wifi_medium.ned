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

import prototype.LocNodes.*;
import prototype.DelayLink;

//
// Generated network with random topology (80 nodes, 120 edges, seed=100).
//
network Wifi_4G_medium extends basic_network {
    parameters:
        int numUsers;
        @display("bgb=400,400;bgg=100,1,grey95");

        @figure[title](type=label; pos=0,-1; anchor=sw; color=darkblue);

        @figure[rcvdPkText](type=indicatorText; pos=310,20; anchor=w; font=,5; textFormat="packets received by user 0: %g"; initialValue=0);
        @statistic[packetReceived](source=users[0].app[0].packetReceived; record=figure(count); targetFigure=rcvdPkText);

    submodules:
        server: RoutingNode {
            parameters:
                @display("p=301,20;i=device/server2");
        }
        users[numUsers]: MobileNode;
        node0 : NeighbourNode;
        node1 : NeighbourNode;
        node2 : NeighbourNode;
        node3 : NeighbourNode;
        node4 : NeighbourNode;
        node5 : NeighbourNode;
        node6 : NeighbourNode;
        node7 : NeighbourNode;
        node8 : NeighbourNode;
        node9 : NeighbourNode;
        node10 : NeighbourNode;
        node11 : NeighbourNode;
        node12 : NeighbourNode;
        node13 : NeighbourNode;
        node14 : NeighbourNode;
        node15 : NeighbourNode;
        node16 : NeighbourNode;
        node17 : NeighbourNode;
        node18 : NeighbourNode;
        node19 : NeighbourNode;
        node20 : NeighbourNode;
        node21 : NeighbourNode;
        node22 : NeighbourNode;
        node23 : NeighbourNode;
        node24 : NeighbourNode;
        node25 : NeighbourNode;
        node26 : NeighbourNode;
        node27 : NeighbourNode;
        node28 : NeighbourNode;
        node29 : NeighbourNode;
        node30 : NeighbourNode;
        node31 : NeighbourNode;
        node32 : NeighbourNode;
        node33 : NeighbourNode;
        node34 : NeighbourNode;
        node35 : NeighbourNode;
        node36 : NeighbourNode;
        node37 : NeighbourNode;
        node38 : NeighbourNode;
        node39 : NeighbourNode;
        node40 : NeighbourNode;
        node41 : NeighbourNode;
        node42 : NeighbourNode;
        node43 : NeighbourNode;
        node44 : NeighbourNode;
        node45 : NeighbourNode;
        node46 : NeighbourNode;
        node47 : NeighbourNode;
        node48 : NeighbourNode;
        node49 : NeighbourNode;
        node50 : NeighbourNode;
        node51 : NeighbourNode;
        node52 : NeighbourNode;
        node53 : NeighbourNode;
        node54 : NeighbourNode;
        node55 : NeighbourNode;
        node56 : NeighbourNode;
        node57 : NeighbourNode;
        node58 : NeighbourNode;
        node59 : NeighbourNode;
        node60 : NeighbourNode;
        node61 : NeighbourNode;
        node62 : NeighbourNode;
        node63 : NeighbourNode;
        node64 : NeighbourNode;
        node65 : NeighbourNode;
        node66 : NeighbourNode;
        node67 : NeighbourNode;
        node68 : NeighbourNode;
        node69 : NeighbourNode;
        node70 : NeighbourNode;
        node71 : NeighbourNode;
        node72 : NeighbourNode;
        node73 : NeighbourNode;
        node74 : NeighbourNode;
        node75 : NeighbourNode;
        node76 : NeighbourNode;
        node77 : NeighbourNode;
        node78 : NeighbourNode;
        node79 : NeighbourNode;
    connections:
        server.pppg++<--> DelayLink <--> node20.pppg++; // Random node to connect to
        node0.pppg++ <--> DelayLink <--> node1.pppg++;
        node0.pppg++ <--> DelayLink <--> node50.pppg++;
        node0.pppg++ <--> DelayLink <--> node67.pppg++;
        node0.pppg++ <--> DelayLink <--> node19.pppg++;
        node0.pppg++ <--> DelayLink <--> node21.pppg++;
        node0.pppg++ <--> DelayLink <--> node6.pppg++;
        node0.pppg++ <--> DelayLink <--> node22.pppg++;
        node0.pppg++ <--> DelayLink <--> node11.pppg++;
        node0.pppg++ <--> DelayLink <--> node75.pppg++;
        node1.pppg++ <--> DelayLink <--> node16.pppg++;
        node1.pppg++ <--> DelayLink <--> node2.pppg++;
        node1.pppg++ <--> DelayLink <--> node3.pppg++;
        node1.pppg++ <--> DelayLink <--> node19.pppg++;
        node1.pppg++ <--> DelayLink <--> node5.pppg++;
        node1.pppg++ <--> DelayLink <--> node62.pppg++;
        node2.pppg++ <--> DelayLink <--> node4.pppg++;
        node2.pppg++ <--> DelayLink <--> node8.pppg++;
        node2.pppg++ <--> DelayLink <--> node30.pppg++;
        node3.pppg++ <--> DelayLink <--> node65.pppg++;
        node3.pppg++ <--> DelayLink <--> node7.pppg++;
        node3.pppg++ <--> DelayLink <--> node10.pppg++;
        node4.pppg++ <--> DelayLink <--> node54.pppg++;
        node4.pppg++ <--> DelayLink <--> node24.pppg++;
        node4.pppg++ <--> DelayLink <--> node27.pppg++;
        node5.pppg++ <--> DelayLink <--> node33.pppg++;
        node5.pppg++ <--> DelayLink <--> node12.pppg++;
        node5.pppg++ <--> DelayLink <--> node14.pppg++;
        node6.pppg++ <--> DelayLink <--> node18.pppg++;
        node6.pppg++ <--> DelayLink <--> node66.pppg++;
        node6.pppg++ <--> DelayLink <--> node51.pppg++;
        node6.pppg++ <--> DelayLink <--> node41.pppg++;
        node6.pppg++ <--> DelayLink <--> node13.pppg++;
        node6.pppg++ <--> DelayLink <--> node47.pppg++;
        node7.pppg++ <--> DelayLink <--> node17.pppg++;
        node7.pppg++ <--> DelayLink <--> node34.pppg++;
        node7.pppg++ <--> DelayLink <--> node39.pppg++;
        node7.pppg++ <--> DelayLink <--> node9.pppg++;
        node7.pppg++ <--> DelayLink <--> node26.pppg++;
        node7.pppg++ <--> DelayLink <--> node74.pppg++;
        node8.pppg++ <--> DelayLink <--> node59.pppg++;
        node8.pppg++ <--> DelayLink <--> node28.pppg++;
        node9.pppg++ <--> DelayLink <--> node48.pppg++;
        node9.pppg++ <--> DelayLink <--> node35.pppg++;
        node9.pppg++ <--> DelayLink <--> node37.pppg++;
        node12.pppg++ <--> DelayLink <--> node52.pppg++;
        node12.pppg++ <--> DelayLink <--> node56.pppg++;
        node12.pppg++ <--> DelayLink <--> node15.pppg++;
        node13.pppg++ <--> DelayLink <--> node69.pppg++;
        node14.pppg++ <--> DelayLink <--> node49.pppg++;
        node14.pppg++ <--> DelayLink <--> node52.pppg++;
        node14.pppg++ <--> DelayLink <--> node38.pppg++;
        node14.pppg++ <--> DelayLink <--> node57.pppg++;
        node14.pppg++ <--> DelayLink <--> node63.pppg++;
        node15.pppg++ <--> DelayLink <--> node20.pppg++;
        node15.pppg++ <--> DelayLink <--> node68.pppg++;
        node15.pppg++ <--> DelayLink <--> node74.pppg++;
        node16.pppg++ <--> DelayLink <--> node79.pppg++;
        node17.pppg++ <--> DelayLink <--> node48.pppg++;
        node17.pppg++ <--> DelayLink <--> node35.pppg++;
        node17.pppg++ <--> DelayLink <--> node25.pppg++;
        node18.pppg++ <--> DelayLink <--> node19.pppg++;
        node19.pppg++ <--> DelayLink <--> node32.pppg++;
        node19.pppg++ <--> DelayLink <--> node20.pppg++;
        node19.pppg++ <--> DelayLink <--> node61.pppg++;
        node19.pppg++ <--> DelayLink <--> node30.pppg++;
        node20.pppg++ <--> DelayLink <--> node29.pppg++;
        node21.pppg++ <--> DelayLink <--> node70.pppg++;
        node22.pppg++ <--> DelayLink <--> node23.pppg++;
        node23.pppg++ <--> DelayLink <--> node41.pppg++;
        node23.pppg++ <--> DelayLink <--> node57.pppg++;
        node23.pppg++ <--> DelayLink <--> node47.pppg++;
        node24.pppg++ <--> DelayLink <--> node76.pppg++;
        node24.pppg++ <--> DelayLink <--> node46.pppg++;
        node24.pppg++ <--> DelayLink <--> node47.pppg++;
        node25.pppg++ <--> DelayLink <--> node71.pppg++;
        node26.pppg++ <--> DelayLink <--> node64.pppg++;
        node26.pppg++ <--> DelayLink <--> node36.pppg++;
        node26.pppg++ <--> DelayLink <--> node73.pppg++;
        node26.pppg++ <--> DelayLink <--> node31.pppg++;
        node27.pppg++ <--> DelayLink <--> node68.pppg++;
        node27.pppg++ <--> DelayLink <--> node54.pppg++;
        node27.pppg++ <--> DelayLink <--> node29.pppg++;
        node28.pppg++ <--> DelayLink <--> node53.pppg++;
        node28.pppg++ <--> DelayLink <--> node60.pppg++;
        node28.pppg++ <--> DelayLink <--> node30.pppg++;
        node29.pppg++ <--> DelayLink <--> node68.pppg++;
        node29.pppg++ <--> DelayLink <--> node40.pppg++;
        node30.pppg++ <--> DelayLink <--> node64.pppg++;
        node30.pppg++ <--> DelayLink <--> node42.pppg++;
        node31.pppg++ <--> DelayLink <--> node45.pppg++;
        node31.pppg++ <--> DelayLink <--> node61.pppg++;
        node32.pppg++ <--> DelayLink <--> node52.pppg++;
        node32.pppg++ <--> DelayLink <--> node72.pppg++;
        node33.pppg++ <--> DelayLink <--> node56.pppg++;
        node33.pppg++ <--> DelayLink <--> node74.pppg++;
        node33.pppg++ <--> DelayLink <--> node77.pppg++;
        node34.pppg++ <--> DelayLink <--> node43.pppg++;
        node35.pppg++ <--> DelayLink <--> node39.pppg++;
        node37.pppg++ <--> DelayLink <--> node56.pppg++;
        node38.pppg++ <--> DelayLink <--> node78.pppg++;
        node39.pppg++ <--> DelayLink <--> node55.pppg++;
        node39.pppg++ <--> DelayLink <--> node44.pppg++;
        node41.pppg++ <--> DelayLink <--> node51.pppg++;
        node41.pppg++ <--> DelayLink <--> node74.pppg++;
        node41.pppg++ <--> DelayLink <--> node75.pppg++;
        node41.pppg++ <--> DelayLink <--> node44.pppg++;
        node43.pppg++ <--> DelayLink <--> node76.pppg++;
        node44.pppg++ <--> DelayLink <--> node64.pppg++;
        node44.pppg++ <--> DelayLink <--> node48.pppg++;
        node47.pppg++ <--> DelayLink <--> node48.pppg++;
        node47.pppg++ <--> DelayLink <--> node76.pppg++;
        node48.pppg++ <--> DelayLink <--> node58.pppg++;
        node52.pppg++ <--> DelayLink <--> node74.pppg++;
        node58.pppg++ <--> DelayLink <--> node63.pppg++;
        node61.pppg++ <--> DelayLink <--> node69.pppg++;
        node63.pppg++ <--> DelayLink <--> node73.pppg++;
        node67.pppg++ <--> DelayLink <--> node79.pppg++;
        node72.pppg++ <--> DelayLink <--> node74.pppg++;
        node73.pppg++ <--> DelayLink <--> node79.pppg++;
        node74.pppg++ <--> DelayLink <--> node75.pppg++;
}
