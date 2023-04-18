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

import prototype.DelayLink;
import prototype.LocNodes.*;

network Wifi_4G extends basic_network
{
    parameters:
        int numUsers;
        int numHomes;

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
        server: RoutingNode {
            parameters:
                @display("p=301,20;i=device/server2");
        }
        backbone: RoutingNode;
        streetCabin: RoutingNode;
        homes[numHomes]: NeighbourNode;
        mobileTower[2]: NeighbourNode;
        users[numUsers]: MobileNode;
    connections:
        server.pppg++ <--> DelayLink <--> backbone.pppg++;
        backbone.pppg++ <--> DelayLink <--> streetCabin.pppg++;
        for i=0..numHomes-1 {
            streetCabin.pppg++ <--> DelayLink <--> homes[i].pppg++;
        }
        for i=0..2-1 {
            backbone.pppg++ <--> DelayLink <--> mobileTower[i].pppg++;
        }
}
