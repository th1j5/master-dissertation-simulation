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

//import inet.networklayer.configurator.ipv4.*;
//import inet.networklayer.configurator.nexthop.*;
import inet.networklayer.configurator.contract.*;
import inet.physicallayer.wireless.common.medium.RadioMedium;
import inet.physicallayer.wireless.unitdisk.*;
import inet.visualizer.contract.IIntegratedVisualizer;

network basic_network {
	parameters:
	    **.ipv4.typename = "TtrIpv4NetworkLayer";
	    @display("bgb=400,400;bgg=100,1,grey95");
	submodules:
        visualizer: <default(firstAvailableOrEmpty("IntegratedCanvasVisualizer"))> like IIntegratedVisualizer if typename != "" {
            @display("p=15,35");
        }
        configurator: <default("")> like IL3NetworkConfigurator if typename != "" {
            @display("p=15,15");
        }
        radioMedium: UnitDiskRadioMedium {
            @display("p=15,55");
        }
}