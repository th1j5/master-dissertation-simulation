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

#ifndef __PROTOTYPE_UDPBASICCONNECTIONAPP_H_
#define __PROTOTYPE_UDPBASICCONNECTIONAPP_H_

#include <omnetpp.h>
#include <inet/applications/udpapp/UdpBasicApp.h>
#include "../LocatorUpdatePacket_m.h"

using namespace omnetpp;

class UdpBasicConnectionApp: public inet::UdpBasicApp, protected cListener {
private:
    cModule* host = nullptr;

protected:
    // parameters
    const char * const locUpdateName = "LocUpdate";
    // statistics
    int numLocUpdateSend = 0;
    int numLocUpdateReceived = 0;

protected:
    //inet::L3Address getCurrentLoc();
    virtual void initialize(int stage) override;
    virtual void processStart() override;
    virtual void processStop() override;
    virtual void receiveSignal(cComponent *source, simsignal_t signalID, cObject *obj, cObject *details) override;

    virtual void sendPacket() override; // only add tag
    void sendLocUpdate(inet::L3Address newLoc);
    virtual void processPacket(inet::Packet *pk) override;

public:
    UdpBasicConnectionApp() {}
    ~UdpBasicConnectionApp();
};

#endif /* __PROTOTYPE_UDPBASICCONNECTIONAPP_H_ */
