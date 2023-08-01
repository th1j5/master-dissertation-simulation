/*
 * ResultFilters.cc
 *
 *  Created on: Mar 31, 2023
 *      Author: thijs
 */
#include "ResultFilters.h"

#include "inet/common/packet/Packet.h"
#include "LocatorUpdatePacket_m.h"
#include "LocUpdatable/LocUpdatable.h"

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
    if (auto adjmgmt = dynamic_cast<LocUpdatable *>(source)) {
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

Register_ResultFilter("lossTime", LossTimeFilter);
void LossTimeFilter::receiveSignal(cResultFilter *prev, simtime_t_cref t, cObject *object, cObject *details) {
    if (auto packet = dynamic_cast<Packet *>(object)) {
        if (auto multiplexPacket = dynamicPtrCast<const MultiplexerPacket>(packet->peekAtFront())) {
            int rerouted = multiplexPacket->getRerouted();
            int seqnum = multiplexPacket->getSequenceNumber();
            intval_t currentDestLoc = (intval_t) multiplexPacket->getLocUpdateCorrelationID();

            // check invariants
            if (!(last_seqnum_old < seqnum))
                throw cRuntimeError("A packet older (or equally old) than the oldest remembered packet has arrived.");
//            ASSERT(oldLocator+1 == newLocator); // a jump was taken
            if (oldLocator < currentDestLoc)
                ASSERT2(last_seqnum_old < seqnum, "A newer locator has an older sequence number?!");
            if (newLocator < currentDestLoc)
                ASSERT2(last_seqnum_new < seqnum, "A newer locator has an older sequence number?!");
            // start of simulation
            if (currentDestLoc == -1)
                return; // skip first packet
            if (oldLocator == -1) {
                if (rerouted != 0)
                    throw cRuntimeError("the first packet to a new locator is already rerouted?!");
                oldLocator = currentDestLoc-1; ASSERT(oldLocator != -1);
                newLocator = currentDestLoc;
                last_seqnum_new = seqnum-1; // update follows
            }
            if (oldLocator <= currentDestLoc && currentDestLoc <= newLocator+1)
                EV_WARN << "A jump in locators detected. Probably due to a locUpdate which wasn't received?" << endl;

            // If a loss time is calculated, fire
            if (newLocator < currentDestLoc) {
                if (rerouted != 0)
                    throw cRuntimeError("the first packet to a new locator is already rerouted?!");
                int lostPackets;
                if (first_seqnum_rerouted_old == -1)
                    lostPackets = first_seqnum_new - last_seqnum_old - 1;
                else
                    lostPackets = first_seqnum_rerouted_old - last_seqnum_old - 1;
                // skip first bogus measurement
                if (last_seqnum_old != -1)
                    fire(this, t, (intval_t) lostPackets, details); //TODO FIXME!
                oldLocator = newLocator;
                newLocator = currentDestLoc;
                last_seqnum_old = last_seqnum_new;
                last_seqnum_new = seqnum;
                first_seqnum_new = seqnum;
                first_seqnum_rerouted_old = first_seqnum_rerouted_new;
                first_seqnum_rerouted_new = -1;
                last_seqnum_rerouted_old = last_seqnum_rerouted_new;
                last_seqnum_rerouted_new = -1;
                return;
            }

            // calculate lossTime (old & new)
            if (currentDestLoc == oldLocator) {
                if (last_seqnum_old + 1 == seqnum) {
                    last_seqnum_old++;
                    return;
                }
                if (rerouted == 0) { // TODO: how do we take this into account in statistics?
                    last_seqnum_old = seqnum;
                    EV_WARN << "Not rerouted, but there is still a gap between packets?? (oldLoc)" << endl;
                    return;
                }
                if (first_seqnum_rerouted_old == -1) {
                    /*first rerouted pkt*/
                    first_seqnum_rerouted_old = seqnum;
                    last_seqnum_rerouted_old = seqnum;
                }
                if (last_seqnum_rerouted_old + 1 == seqnum) {
                    last_seqnum_rerouted_old++;
                    return;
                }
            }
            if (currentDestLoc == newLocator) {
                if (last_seqnum_new + 1 == seqnum) {
                    last_seqnum_new++;
                    return;
                }
                if (rerouted == 0) { // TODO: how do we take this into account in statistics?
                    last_seqnum_new = seqnum;
                    EV_WARN << "Not rerouted, but there is still a gap between packets??" << endl;
                    return;
                }
                if (first_seqnum_rerouted_new == -1) {
                    /*first rerouted pkt*/
                    first_seqnum_rerouted_new = seqnum;
                    last_seqnum_rerouted_new = seqnum;
                }
                if (last_seqnum_rerouted_new + 1 == seqnum) {
                    last_seqnum_rerouted_new++;
                    return;
                }
            }

            throw cRuntimeError("unhandled case for calculating loss Time");
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
