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

package prototype.UdpBasicConnectionApp;

import inet.applications.udpapp.*;
import inet.applications.contract.*;

//
// TODO auto-generated module
//
simple UdpBasicConnectionApp extends UdpBasicApp
{
    parameters:
        @class(UdpBasicConnectionApp);
        @signal[locatorUpdateSent](type=inet::Packet);
        @signal[locatorUpdateReceived](type=inet::Packet);
        @signal[oldLocatorUnreachable](type=bool);
        @statistic[locatorUpdateSent](title="locUpdates sent"; record=count,vector(locUpdateCorrelation); interpolationmode=none);
        @statistic[locatorUpdateReceived](title="locUpdates received"; record=count,vector(locUpdateCorrelation); interpolationmode=none);
        @statistic[endToEndDelay](title="end-to-end delay"; source="dataAge(packetReceived)"; unit=s; record=histogram,vector; interpolationmode=none);
        @statistic[packetJitter](title="packet jitter"; source="jitter(dataAge(packetReceived))"; unit=s; record=histogram,vector; interpolationmode=none);
        @statistic[oldLocatorUnreachable](title="old locator becomes unreachable";record=vector; interpolationmode=none;);
        string newLocInterface = default(""); // which NIC should be listed to for Loc updates
        string oldLocInterface = default(""); // NIC to listen to for unreachability of old Loc
}
