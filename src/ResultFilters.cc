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
        // TODO Should use Dissector, but I don't understand that completely
        if (auto multiplexPacket = dynamicPtrCast<const MultiplexerPacket>(packet->peekAtFront())) {
            fire(this, t, multiplexPacket->getLocUpdateCorrelationID(), details);
        }
        else {
            // TODO: replace outer if/else by while/if (once we are sure)
            while (!dynamicPtrCast<const MultiplexerPacket>(packet->peekAtFront())) {
                packet->popAtFront();
            }
            if (auto multiplexPacket = dynamicPtrCast<const MultiplexerPacket>(packet->peekAtFront())) {
                fire(this, t, multiplexPacket->getLocUpdateCorrelationID(), details);
            }
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

Register_ResultFilter("rerouted", ReroutedFilter);
void ReroutedFilter::receiveSignal(cResultFilter *prev, simtime_t_cref t, cObject *object, cObject *details) {
    if (auto packet = dynamic_cast<Packet *>(object)) {
        if (auto multiplexPacket = dynamicPtrCast<const MultiplexerPacket>(packet->peekAtFront())) {
            fire(this, t, (intval_t) multiplexPacket->getRerouted(), details);
        }
    }
}

Register_ResultFilter("udpData", UDPDataFilter);
void UDPDataFilter::receiveSignal(cResultFilter *prev, simtime_t_cref t, cObject *object, cObject *details) {
    if (auto packet = dynamic_cast<Packet *>(object)) {
        if (packetFilter->matches(packet))
            fire(this, t, object, details);
    }
}
void UDPDataFilter::init(Context *ctx) {
    cObjectResultFilter::init(ctx);
    packetFilter = new PacketFilter();
    packetFilter->setPattern("UDPData*");
}
UDPDataFilter::~UDPDataFilter() {
    delete packetFilter;
}
