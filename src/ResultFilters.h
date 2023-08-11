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

class LossTimeFilter: public cObjectResultFilter {
  protected:
    int last_seqnum_old = -1;
    int last_seqnum_new = -1;
    int first_seqnum_new = -1;
    int first_seqnum_rerouted_old = -1;
    int first_seqnum_rerouted_new = -1;
    int last_seqnum_rerouted_old = -1;
    int last_seqnum_rerouted_new = -1;
    intval_t oldLocator = -1;
    intval_t newLocator = 0;
  public:
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Woverloaded-virtual"
    virtual void receiveSignal(cResultFilter *prev, simtime_t_cref t, cObject *object, cObject *details) override;
    #pragma clang diagnostic pop
};

class LossTimeRecorder: public HistogramRecorder {
  protected:
    // Assumption: the first packet has the lowest LocCorrID
    intval_t locator_offset = -1;
    struct loc_stats {
        //int first_seqnum_rerouted = -1; //
        int first_seqnum = std::numeric_limits<int>::max();
        simtime_t first_time = SimTime::getMaxTime();
        int lost_pkt = 0; // packets which are lost in a normal stream...
        int out_of_order_pkt = 0; // out-of-order is increased when a pkt with lower seqnum arrives in normal stream...
        int last_seqnum = -1;
        simtime_t last_time = 0;
    };
    std::vector<loc_stats> locator;
    virtual void init(Context *ctx) override;
    virtual void finish(cResultFilter *prev) override;
  public:
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Woverloaded-virtual"
    virtual void receiveSignal(cResultFilter *prev, simtime_t_cref t, cObject *object, cObject *details) override;
    #pragma clang diagnostic pop
};

//class HistogramArrivalRecorder: public cKSplit {
//  protected:
//    virtual void init(Context *ctx) override {
//        HistogramRecorder::init(ctx);
//        locator = std::vector<loc_stats>(100);
//    };
//    virtual void finish(cResultFilter *prev) override;
//  public:
//    #pragma clang diagnostic push
//    #pragma clang diagnostic ignored "-Woverloaded-virtual"
//    virtual void receiveSignal(cResultFilter *prev, simtime_t_cref t, cObject *object, cObject *details) override;
//    #pragma clang diagnostic pop
//};

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

class NodeIDFilter: public cObjectResultFilter {
  public:
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Woverloaded-virtual"
    virtual void receiveSignal(cResultFilter *prev, simtime_t_cref t, cObject *object, cObject *details) override;
    #pragma clang diagnostic pop
};

class ArrivalTimeFilter: public cObjectResultFilter {
  public:
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Woverloaded-virtual"
    virtual void receiveSignal(cResultFilter *prev, simtime_t_cref t, cObject *object, cObject *details) override;
    #pragma clang diagnostic pop
};

class StopBandFilter: public cNumericResultFilter {
  public:
    virtual bool process(simtime_t& t, double& value, cObject *details) override;
};
#endif /* RESULTFILTERS_H_ */
