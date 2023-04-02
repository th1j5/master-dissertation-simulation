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

package prototype.LocNodes;

import inet.node.inet.*;
import inet.applications.contract.IApp;

//
// TODO auto-generated module
//
module RoutingNode extends Router
{
    parameters:
        *.adjacencyMgmt = default(absPath(".adjacencyManager"));
    submodules:
        adjacencyManager: <default("")> like IApp if typename != "" {      // AdjMgmt over UDP/TCP protocol
            @display("p=700,75");
        }
    connections:
        adjacencyManager.socketOut --> at.in++ if exists(adjacencyManager);
        adjacencyManager.socketIn <-- at.out++ if exists(adjacencyManager);
}
// Can get new Loc (doesn't HAS to mean that it moves)
module MobileNode extends ManetRouter {
    parameters:
        *.adjacencyMgmt = default(absPath(".adjacencyManager"));
    submodules:
        adjacencyManager: <default("AdjacencyManagerClient")> like IApp if typename != "" {      // AdjMgmt over UDP/TCP protocol
            @display("p=700,75");
        }
    connections:
        adjacencyManager.socketOut --> at.in++ if exists(adjacencyManager);
        adjacencyManager.socketIn <-- at.out++ if exists(adjacencyManager);
}
