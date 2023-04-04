/*
 * ResultFilters.h
 *
 *  Created on: Mar 31, 2023
 *      Author: thijs
 */

#ifndef RESULTFILTERS_H_
#define RESULTFILTERS_H_

#include <omnetpp/cresultfilter.h>

using namespace omnetpp;

/**
 * Filter that expects a Packet and outputs the locUpdate correlationID.
 */
class LocUpdatesFilter: public cObjectResultFilter {
  public:
    virtual void receiveSignal(cResultFilter *prev, simtime_t_cref t, cObject *object, cObject *details) override;
    virtual void receiveSignal(cComponent *source, simsignal_t signalID, intval_t l, cObject *details) override;
    virtual void receiveSignal(cResultFilter *prev, simtime_t_cref t, double d, cObject *details) override;
};

#endif /* RESULTFILTERS_H_ */
