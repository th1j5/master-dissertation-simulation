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

network Wifi_4G_country1 extends basic_network
{
    parameters:
        @display("bgb=4000,4000;bgg=100,1,grey95");
        int numUsers;
        int numHomesPerStreet;
        int numStreetsPerCity;
        int numCities;

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
        users[numUsers]: MobileNode {
            @display("p=27.692308,210.76924");
        }
        providerY: RoutingNode {
            @display("p=264.6154,121.53847");
        }
        mobileTower[2]: NeighbourNode {
            @display("p=196.15385,356.15387,,300");
        }
        IXP: RoutingNode {
            @display("p=356.9231,328.46155");
        }
        providerX: RoutingNode {
            @display("p=308.46155,356.9231");
        }
        backbone: RoutingNode {
            @display("p=366.9231,122.30769");
        }
        homes[numHomesPerStreet*numStreetsPerCity*numCities]: NeighbourNode;
        streetCabin[numStreetsPerCity*numCities]: RoutingNode;
        cityCabin[numCities]: RoutingNode;
    connections:
        server.pppg++ <--> DelayLink <--> providerY.pppg++;
        for i=0..2-1 {
            mobileTower[i].pppg++ <--> DelayLink <--> providerX.pppg++;
        }
        providerY.pppg++ <--> DelayLink <--> backbone.pppg++;
        IXP.pppg++ <--> DelayLink <--> backbone.pppg++;
        IXP.pppg++ <--> DelayLink <--> providerX.pppg++;
        for i=0..numCities-1 {
            providerY.pppg++ <--> DelayLink <--> cityCabin[i].pppg++;        
        }
        for i=0..numCities-1, for j=0..numStreetsPerCity-1 {
            cityCabin[i].pppg++ <--> DelayLink <--> streetCabin[i*numStreetsPerCity+j].pppg++;
        }
        for i=0..numCities-1, for j=0..numStreetsPerCity-1, for k=0..numHomesPerStreet-1 {
            streetCabin[i*numStreetsPerCity+j].pppg++ <--> DelayLink <--> homes[i*numHomesPerStreet*numStreetsPerCity+j*numHomesPerStreet+k].pppg++;
        }
}
