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

//package prototype.AdjacencyManager;
package prototype.OldAdjMgmt;

import inet.applications.contract.IApp;

//
// TODO auto-generated module
//
simple AdjacencyManagerClient extends AdjacencyManager like IApp
{
    parameters:
        @class(AdjacencyManagerClient);
        @signal[neighLocatorUpdateSent](type=inet::Packet);
        @signal[oldLocatorUnreachable](type=int); // int numLocUpdates
        @signal[newLocatorAssigned](type=int); // int numLocUpdates
        @statistic[oldLocatorUnreachable](title="old locator becomes unreachable";record=vector(locUpdateCorrelation); interpolationmode=none;);
        @statistic[newLocatorAssigned](title="new locator assigned";record=vector(locUpdateCorrelation); interpolationmode=none;);
        string arpModule = default("^.ipv4.arp");
        double getLocInterval @unit(s) = default(0.01s);
}
