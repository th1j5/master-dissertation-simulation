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

#include "LocUpdatable.h"

//using namespace inet; // more OK to use in .cc

const simsignal_t LocUpdatable::newLocAssignedSignal = cComponent::registerSignal("newLocatorAssigned");
const simsignal_t LocUpdatable::oldLocRemovedSignal  = cComponent::registerSignal("oldLocatorUnreachable");

LocUpdatable::LocUpdatable() {
//    ASSERT(strcmp(par("locChangingStrategy"), "end2end") == 0);
}

LocUpdatable::~LocUpdatable() {
    // TODO Auto-generated destructor stub
}

