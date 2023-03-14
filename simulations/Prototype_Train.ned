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

network PrototypeTrain
{
    parameters:
        int numPassengers;
        int numRoutersAlongTrainTrack;
        @display("bgb=698,358");
    submodules:
        configurator: Ipv4NetworkConfigurator {
            @display("p=37,31");
        }
        radioMedium: UnitDiskRadioMedium {
            @display("p=37,96");
        }
        server: StandardHost {
            @display("p=301,20");
        }
        routersAlongTrack[numRoutersAlongTrainTrack]: Router {
            parameters:
            	//@display("p=400+50*numRoutersAlongTrainTrack,300+50*numRoutersAlongTrainTrack;r=,,#707070");
        }
        trainTransceiver: Router {
            @display("p=298,219");
        }
        passenger[numPassengers]: StandardHost {
            @display("p=,200,r");
        }
    connections:
        for i=0..numRoutersAlongTrainTrack-1 {
            server.pppg++ <--> {  datarate = 1Gbps; } <--> routersAlongTrack[i].pppg++;
        }
        // First connection
        // routersAlongTrack[0].pppg++ <--> {  datarate = 1Gbps; } <--> trainTransceiver.pppg++;
        for i=0..numPassengers-1 {
            passenger[i].pppg++ <--> {  datarate = 1Gbps; } <--> trainTransceiver.pppg++;
        }
}

network PrototypeRandom
{
    parameters:
        int numClients; // number of clients in the network
    submodules:
        configurator: Ipv4NetworkConfigurator;
        server: StandardHost;
        router: Router;
        switch: EthernetSwitch;
        client[numClients]: StandardHost;
    connections: // network level connections
        router.pppg++ <--> {  datarate = 1Gbps; } <--> server.pppg++; // PPP
        switch.ethg++ <--> Eth1G <--> router.ethg++; // bidirectional ethernet
        for i=0..numClients-1 {
            client[i].ethg++ <--> Eth1G <--> switch.ethg++; // ethernet
        }
}
