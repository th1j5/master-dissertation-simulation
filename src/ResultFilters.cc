/*
 * ResultFilters.cc
 *
 *  Created on: Mar 31, 2023
 *      Author: thijs
 */

#include "ResultFilters.h"

#include "inet/common/packet/Packet.h"
#include "LocatorUpdatePacket_m.h"

using namespace inet;

Register_ResultFilter("locUpdateCorrelation", LocUpdatesFilter);

void LocUpdatesFilter::receiveSignal(cResultFilter *prev, simtime_t_cref t, cObject *object, cObject *details)
{
    if (auto packet = dynamic_cast<Packet *>(object)) {
        if (auto locUpdPacket = dynamicPtrCast<const LocatorUpdatePacket>(packet->peekAtFront())) {
            fire(this, t, locUpdPacket->getLocUpdateCorrelationID(), details);
        }
    }
}
