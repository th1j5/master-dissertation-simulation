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

import inet.networklayer.common.L3Address;

cplusplus {{
#include <stack>
typedef std::stack<inet::L3Address, std::vector<inet::L3Address>> RoutingPath;

std::ostream& operator<<(std::ostream& os, RoutingPath e);
}}

cplusplus(cc) {{
std::ostream& operator<<(std::ostream& os, RoutingPath e) {
    while (!e.empty()) {
        os << e.top();
        e.pop();
    }
    return os;
}
}}

class RoutingPath {
    @existingClass;
    //@opaque; //TODO?
    // Would be needed to silence warning, only import for the descriptor class
    // https://doc.omnetpp.org/omnetpp/manual/#sec:msg-defs:descriptor-classes
//    @toValue(.str());  // use IPAddress::str() to produce a string
//    @fromValue(IPAddress($));  // use constructor; '$' will be replaced by the string
}

