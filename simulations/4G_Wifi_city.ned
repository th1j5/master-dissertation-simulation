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

network Wifi_4G_city1 extends basic_network
{
    parameters:
        @display("bgb=600,600;bgg=100,1,grey95");
        int numUsers;
        int numHomesPerStreet;
        int numStreetsPerQuarter;
        int numQuarters;
        int num4G;

        @figure[title](type=label; pos=0,-1; anchor=sw; color=darkblue);

        @figure[rcvdPkText](type=indicatorText; pos=0,-40; anchor=w; font=,5; textFormat="packets received by user 0: %g"; initialValue=0);
        @figure[sentPkText](type=indicatorText; pos=0,-30; anchor=w; font=,5; textFormat="packets send to user 0: %g"; initialValue=0);
        @figure[sentLocUpdText](type=indicatorText; pos=0,-20; anchor=w; font=,5; textFormat="Loc updates send from user 0 to server: %g"; initialValue=0);
        @figure[rcvdLocUpdText](type=indicatorText; pos=0,-10; anchor=w; font=,5; textFormat="Loc updates received by server (by user?): %g"; initialValue=0);
        @statistic[packetReceived](source=users[0].app[0].packetReceived; record=figure(count); targetFigure=rcvdPkText);
        @statistic[packetSent](source=server.app[0].packetSent; record=figure(count); targetFigure=sentPkText);
        @statistic[locUpdSent](source=users[0].app[0].locatorUpdateSent; record=figure(count); targetFigure=sentLocUpdText);
        @statistic[locUpdRcvd](source=server.app[0].locatorUpdateReceived; record=figure(count); targetFigure=rcvdLocUpdText);
    submodules:
        server: RoutingNode {
            parameters:
                @display("p=300,-100;i=device/server");
        }
        users[numUsers]: MobileNode {
            @display("p=27.692308,210.76924");
        }
        providerY: RoutingNode {
            @display("p=300,-30");
        }
        mobileTower[num4G]: NeighbourNode {
            @display("i=device/antennatower_l");
        }
        IXP: RoutingNode {
            //@display("p=356.9231,328.46155");
        }
        providerX: RoutingNode {
            @display("p=400,-30");
        }
        backbone: RoutingNode {
            //@display("p=366.9231,122.30769");
        }
        homes[numHomesPerStreet*numStreetsPerQuarter*numQuarters]: NeighbourNode {
            @display("i=device/accesspoint_s");
        }
        streetCabin[numStreetsPerQuarter*numQuarters]: RoutingNode {
            @display("i=abstract/router_vs");
        }
        quarterCabin[numQuarters]: RoutingNode {
            @display("i=abstract/router_s");
        }
    connections:
        server.pppg++ <--> DelayLink {delay=30ms;} <--> providerY.pppg++;
        for i=0..2-1 {
            mobileTower[i].pppg++ <--> DelayLink <--> providerX.pppg++;
        }
        providerY.pppg++ <--> DelayLink {delay=15ms;} <--> backbone.pppg++;
        IXP.pppg++ <--> DelayLink <--> backbone.pppg++;
        IXP.pppg++ <--> DelayLink <--> providerX.pppg++;
        for i=0..numQuarters-1 {
            providerY.pppg++ <--> DelayLink <--> quarterCabin[i].pppg++;
        }
        for i=0..numQuarters-1, for j=0..numStreetsPerQuarter-1 {
            quarterCabin[i].pppg++ <--> DelayLink <--> streetCabin[i*numStreetsPerQuarter+j].pppg++;
        }
        for i=0..numQuarters-1, for j=0..numStreetsPerQuarter-1, for k=0..numHomesPerStreet-1 {
            streetCabin[i*numStreetsPerQuarter+j].pppg++ <--> DelayLink <--> homes[i*numHomesPerStreet*numStreetsPerQuarter+j*numHomesPerStreet+k].pppg++;
        }
}
