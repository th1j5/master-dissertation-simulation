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
package prototype.LocAssignAlgo;

import inet.applications.contract.IApp;

//
// TODO auto-generated module
//
simple AdjacencyManagerServer extends AdjacencyManager like IApp
{
    @class(AdjacencyManagerServer);
    @signal[neighLocatorUpdateReceived](type=inet::Packet);
    @statistic[neighLocatorUpdateReceived](title="locUpdates received";record=vector(locUpdateCorrelation); interpolationmode=none);
    string transientTriangularRoutingModule = default("^.ipv4.ttrTable");
    int maxNumClients = default(-1); // maximum number of clients (IPs) allowed to be leased
}
