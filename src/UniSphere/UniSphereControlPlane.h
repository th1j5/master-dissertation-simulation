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

#ifndef UNISPHERE_UNISPHERECONTROLPLANE_H_
#define UNISPHERE_UNISPHERECONTROLPLANE_H_

#include <omnetpp.h>
#include "inet/routing/base/RoutingProtocolBase.h"

using namespace omnetpp;

class UniSphereControlPlane: public inet::RoutingProtocolBase, protected omnetpp::cListener {
  public:
    UniSphereControlPlane();
    virtual ~UniSphereControlPlane();

  protected:
    virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    virtual void initialize(int stage) override {}
    virtual void handleMessageWhenUp(cMessage *msg) override {}
    virtual void receiveSignal(cComponent *source, simsignal_t signalID, cObject *obj, cObject *details) override {}
    virtual void announceOurselves();

    // lifecycle
    virtual void handleStartOperation(inet::LifecycleOperation *operation) override {}
    virtual void handleStopOperation(inet::LifecycleOperation *operation) override {}
    virtual void handleCrashOperation(inet::LifecycleOperation *operation) override {}

};

#endif /* UNISPHERE_UNISPHERECONTROLPLANE_H_ */
