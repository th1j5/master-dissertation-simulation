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

#ifndef LOCUPDATABLE_LOCUPDATABLE_H_
#define LOCUPDATABLE_LOCUPDATABLE_H_

#include <omnetpp.h>

using namespace omnetpp;

class LocUpdatable {
  public:
    static simsignal_t newLocAssignedSignal;
    LocUpdatable();
    virtual ~LocUpdatable();

  protected:
    // statistics
    int numLocUpdates = -1; // First send is 0
    int numLocUpdateSend = 0;
    int numNewNeighConnected = 0;


    // parameters
    cModule *host = nullptr;

  public:
    // FIXME: encoding ints mostly works, but not necessarily (https://stackoverflow.com/questions/10749419/encode-multiple-ints-into-a-double)
    double getCorrID(int numLocUpdates) {return (((int64_t)host->getId())<<32) | ((int64_t)numLocUpdates);};
    int getNumLocUpdates() {return numLocUpdates;};
};

#endif /* LOCUPDATABLE_LOCUPDATABLE_H_ */
