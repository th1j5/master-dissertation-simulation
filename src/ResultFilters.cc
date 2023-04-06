/*
 * ResultFilters.cc
 *
 *  Created on: Mar 31, 2023
 *      Author: thijs
 */

#include "ResultFilters.h"

#include "inet/common/packet/Packet.h"
#include "LocatorUpdatePacket_m.h"
#include "AdjacencyManager/AdjacencyManagerClient.h"

using namespace inet;

Register_ResultFilter("locUpdateCorrelation", LocUpdatesFilter);
void LocUpdatesFilter::receiveSignal(cResultFilter *prev, simtime_t_cref t, cObject *object, cObject *details)
{
    if (auto packet = dynamic_cast<Packet *>(object)) {
        if (auto multiplexPacket = dynamicPtrCast<const MultiplexerPacket>(packet->peekAtFront())) {
            fire(this, t, multiplexPacket->getLocUpdateCorrelationID(), details);
        }
    }
}
void LocUpdatesFilter::receiveSignal(cResultFilter *prev, simtime_t_cref t, double d, cObject *details) {
    // rejoice! You just succeeded in beating the system into submission
    fire(this, t, d, details);
}
void LocUpdatesFilter::receiveSignal(cComponent *source, simsignal_t signalID, intval_t numLocUpdates, cObject *details) {
    // intercept, check if AdjMgmt, then continue up the hierarchy
    if (auto adjmgmt = dynamic_cast<AdjacencyManagerClient *>(source)) {
        // fire(this, ?, adjmgmt->getCorrID(numLocUpdates), details);
        cResultFilter::receiveSignal(source, signalID, adjmgmt->getCorrID(numLocUpdates), details);
    }
    else
        cResultFilter::receiveSignal(source, signalID, numLocUpdates, details);
}
