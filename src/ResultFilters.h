/*
 * ResultFilters.h
 *
 *  Created on: Mar 31, 2023
 *      Author: thijs
 */

#ifndef RESULTFILTERS_H_
#define RESULTFILTERS_H_

#include <omnetpp/cresultfilter.h>
#include "inet/common/packet/PacketFilter.h"

using namespace omnetpp;

/**
 * Filter that expects a Packet and outputs the locUpdate correlationID.
 */
class LocUpdatesFilter: public cObjectResultFilter {
  public:
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Woverloaded-virtual"
    virtual void receiveSignal(cResultFilter *prev, simtime_t_cref t, cObject *object, cObject *details) override;
    virtual void receiveSignal(cComponent *source, simsignal_t signalID, intval_t l, cObject *details) override;
    virtual void receiveSignal(cResultFilter *prev, simtime_t_cref t, double d, cObject *details) override;
#pragma clang diagnostic pop
};

class ReroutedFilter: public cObjectResultFilter {
  public:
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Woverloaded-virtual"
    virtual void receiveSignal(cResultFilter *prev, simtime_t_cref t, cObject *object, cObject *details) override;
#pragma clang diagnostic pop
};

class UDPDataFilter: public cObjectResultFilter {
  protected:
    inet::PacketFilter *packetFilter;
  public:
    ~UDPDataFilter();
    virtual void init(Context *ctx) override;
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Woverloaded-virtual"
    virtual void receiveSignal(cResultFilter *prev, simtime_t_cref t, cObject *object, cObject *details) override;
#pragma clang diagnostic pop
};

#endif /* RESULTFILTERS_H_ */
