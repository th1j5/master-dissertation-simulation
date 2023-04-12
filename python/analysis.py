import math
import numpy as np
import pandas as pd
import itertools
from omnetpp.scave import results, chart, utils

def process(props):
    "Collect and process data"

    # collect parameters for query
    filter_expression = props["filter"]
    start_time = float(props["vector_start_time"] or -math.inf)
    end_time = float(props["vector_end_time"] or math.inf)

    # query vector data into a data frame
    try:
        df = results.get_vectors(filter_expression, include_attrs=True, include_runattrs=True, include_itervars=True, start_time=start_time, end_time=end_time)
    except results.ResultQueryError as e:
        raise chart.ChartScriptError("Error while querying results: " + str(e))

    if df.empty:
        raise chart.ChartScriptError("The result filter returned no data.")

    print(df)
    print(df["vectime"][0])
    print(df.info())
    print(df.to_csv()) # df = pd.read_clipboard(sep=',')

    gather_data(df)
    #slow_gather_data(df)

    # apply vector operations
    df = utils.perform_vector_ops(df, props["vector_operations"])
    return df

############### experiment ###########
def gather_data(df):
    print(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> start gather data <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<")
    # 1 row voor elke MN
    newLocRows = df.loc[df["name"].str.contains("newLocatorAssigned:vector")].reset_index(drop=True)
    locRemoveRows = df.loc[df["name"].str.contains("oldLocatorUnreachable:vector")].reset_index(drop=True)
    neighRows = df.loc[df["name"].str.contains("neighLocatorUpdateReceived:vector")].reset_index(drop=True)
    # meerdere rows voor elke server per client
    rcvdRows = df.loc[df["name"].str.contains("locatorUpdateReceived:vector")].reset_index(drop=True)
    pktDelayRows = df.loc[df["name"].str.contains("endToEndDelay:vector")].reset_index(drop=True)
    pktCorrIDRows = df.loc[df["name"].str.contains("corrIDTag:vector")].reset_index(drop=True)
    pktReroutedRows = df.loc[df["name"].str.contains("reroutedTag:vector")].reset_index(drop=True)
    pktLossRows = df.loc[df["name"].str.contains("endToEndDelayLost:vector")].reset_index(drop=True)
    pktCorrIDLossRows = df.loc[df["name"].str.contains("corrIDLost:vector")].reset_index(drop=True)
    print(f"{newLocRows['vecvalue'].iloc[0][-1]=}")
    print(f"{rcvdRows['vecvalue'].iloc[0][-1]=}")
    # .eq: can't determine equality of numpy arrays - .equals: can't handle mismatching numpy arrays (index out of bounds)
    try:
        gelijk = newLocRows["vecvalue"].equals(rcvdRows["vecvalue"])
    except:
        gelijk = False
    if not gelijk:
        print("There is a mismatch between newLoc and rcvdRows") # assume only 1 MN and 1 CN
        newLocRows.iloc[0]["vectime"]  = newLocRows.iloc[0]["vectime"][:-1]
        newLocRows.iloc[0]["vecvalue"] = newLocRows.iloc[0]["vecvalue"][:-1]
        pd.testing.assert_series_equal(newLocRows["vecvalue"], rcvdRows["vecvalue"])
    if not newLocRows["vecvalue"].equals(locRemoveRows["vecvalue"]):
        pass
#        print("There is a mismatch between newLoc and locRemoveRows") # assume only 1 MN and 1 CN
#        locRemoveRows["vecvalue"].iloc[0] = locRemoveRows["vecvalue"].iloc[0][:-1]
#        pd.testing.assert_series_equal(newLocRows["vecvalue"], rcvdRows["vecvalue"])
    assert pktDelayRows["vectime"].equals(pktCorrIDRows["vectime"])
    assert pktDelayRows["vectime"].equals(pktReroutedRows["vectime"])
    assert np.all([np.all(MN<=1) for MN in pktReroutedRows["vecvalue"]]), "damn, you got packets which rerouted more than once!"
    if not neighRows.index.equals(pktLossRows.index):
        print("The amount of neighbours with updates is different from the amount of neighbours with packet loss")
        print(f"{neighRows=}")
        print(f"{pktLossRows=}")
    assert pktLossRows["vectime"].equals(pktCorrIDLossRows["vectime"])

    time = newLocRows["vectime"]
    # dimensions
    numMN = newLocRows.shape[0] # Mobile Nodes
    numLocUpdates = newLocRows["vectime"].apply(np.size) # Series
    numCN = np.nan # Corresponding Nodes (for each LocUpdate)
    # data structures
    # FIXME: assumption: only 1 neigh or CN
    dim = (numMN, np.max(numLocUpdates))
    timeOldLocRemove = np.empty(dim)
    timeCNUpdate     = np.empty(dim)
    timeNNUpdate     = np.empty(dim)
    NNDelay          = np.empty(dim)
    lossTime         = np.empty(dim)
    reroutingTime    = np.empty(dim)
    delay            = np.empty((3, *dim)) # before, during, after
    timeOldLocRemove[:] = np.nan
    timeCNUpdate[:]     = np.nan
    timeNNUpdate[:]     = np.nan
    NNDelay[:]          = np.nan
    lossTime[:]         = np.nan
    reroutingTime[:]    = np.nan
    delay[:]            = np.nan

    for MN_i, (MN, MN_old, CN, pktCorrID, pktRR, pktDelay) in enumerate(
    zip(newLocRows.itertuples(), locRemoveRows.itertuples(), rcvdRows.itertuples(), pktCorrIDRows.itertuples(), pktReroutedRows.itertuples(), pktDelayRows.itertuples())):
        corrIDs = MN.vecvalue
        #### locRemove
        corrMatrix = MN_old.vecvalue == corrIDs[:,np.newaxis]
        # ASSUMPTION: there is always at least one true value (see usage argmax)
        if np.any(corrMatrix.sum(axis=1) != 1):
            print(f"Warning: oldLocatorUnreachable has {corrMatrix.sum(axis=1)=}, this probably means that it was unreachable for an amount of time")
            print(f"Check correct behaviour of this case")
            # FIXME: check on rebound off wall how losstime is calculated
        # should be earliest, thus 'left' or argmax
        # idx = np.searchsorted(MN_old.vecvalue, corrIDs, side='left')
        idx = corrMatrix.argmax(axis=1)
        timeOldLocRemove[MN_i] = MN_old.vectime[idx]
        #### CN
        timeCNUpdate[MN_i] = CN.vectime[CN.vecvalue == corrIDs]
        if not np.all(CN.vecvalue == corrIDs):
            print(f"Warning: {CN.vecvalue == corrIDs=}")
        #### neigh
        for NN, pktNNDelay, pktNNCorrID in itertools.zip_longest(neighRows.itertuples(), pktLossRows.itertuples(), pktCorrIDLossRows.itertuples()):
            corrMatrix = NN.vecvalue == corrIDs[:,np.newaxis]
            corrID_idx, neigh_idx = np.nonzero(corrMatrix)
            assert len(np.unique(corrID_idx)) == len(corrID_idx), f"Warning: in {NN=} we have {corrMatrix=}"
                #print(f"This should be wrong, no corrID can have different updates")
            assert len(np.unique(neigh_idx)) == len(neigh_idx), f"Warning: in {NN=} we have {corrMatrix=}"
                #print(f"This should be wrong, no update can have different corrIDs")
            timeNNUpdate[MN_i][corrID_idx] = NN.vectime[neigh_idx]
            print(f"{corrID_idx=}")
            ## delay of lost packets
            #corrMatrixLoss = pktNNCorrID.vecvalue == corrIDs[:,np.newaxis]
            if not pktNNDelay == None:
                filterDelayBeforeNNIdx = np.searchsorted(pktNNDelay.vectime, timeNNUpdate[MN_i][corrID_idx], side="right")-1
                print(f"{filterDelayBeforeNNIdx=}")
                #print(f"{NNDelay[MN_i][:]=}")
                NNDelay[MN_i][corrID_idx][filterDelayBeforeNNIdx>=0] = pktNNDelay.vecvalue[filterDelayBeforeNNIdx][filterDelayBeforeNNIdx>=0]
        #### delay
        corrIDs_before = np.concatenate(([np.nan], corrIDs[:-1]))
        corrMatrixBefore = pktCorrID.vecvalue == corrIDs_before[:,np.newaxis]
        corrMatrixAfter  = pktCorrID.vecvalue == corrIDs[:,np.newaxis]
        ## before - FIXME assumption: packets are not rerouted before the old loc is removed
        filterDelayBeforeIdx = np.searchsorted(pktDelay.vectime, timeOldLocRemove[MN_i], side="right") -1 # TODO check side parameter
        # don't assign when negative idx
        print(pktDelay.vectime[filterDelayBeforeIdx][filterDelayBeforeIdx >= 0])
        delay[0,MN_i][filterDelayBeforeIdx>=0] = pktDelay.vecvalue[filterDelayBeforeIdx][filterDelayBeforeIdx >= 0]
        ## during
        filterDelayDuringIdx = ~np.logical_and(corrMatrixBefore, pktRR.vecvalue > 0)
        delay_b= np.broadcast_to(pktDelay.vecvalue, filterDelayDuringIdx.shape)
        delay_mean = np.ma.masked_where(filterDelayDuringIdx, delay_b).mean(axis=1)
        delay[1,MN_i] = delay_mean.filled(np.nan)
        ## after
        corrID_idx, time_idx = argfind(np.logical_and(corrMatrixAfter, pktRR.vecvalue == 0))
        delay[2,MN_i][corrID_idx] = pktDelay.vecvalue[time_idx]
    print(f"Warning: check delay data gathering")
    print(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> stats <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<")
    pd.set_option('display.max_colwidth', None)  # or 1000
    print(f"time={time}")
    print(f"{timeOldLocRemove=}")
    print(f"{timeNNUpdate=}")
    print(f"{NNDelay=}")
    print(f"{timeCNUpdate=}")
    print(f"{delay=}")

    #pktNNDelay = np.array([0.2,0.2,0.2,0.2,0.2,0.2,0.2,0.2,0.2,0.2,0.2,0.2,0.2]) # FIXME: WRONG HARDCODED
    #FIXME: if NNDelay == nan (due to no packets dropped/lost, are our calculations still correct?)
    timeCNRerouting = timeNNUpdate - NNDelay
    assert np.all((timeCNRerouting < timeCNUpdate) | np.isnan(timeCNRerouting)), f"Not normal: {timeCNRerouting=} < {timeCNUpdate=}"
    lossTime = np.maximum(0, np.where(~np.isnan(timeCNRerouting), timeCNRerouting, timeCNUpdate) - (timeOldLocRemove - delay[0]))
    reroutingTime = timeCNUpdate - timeCNRerouting
    print(f"{timeOldLocRemove - delay[0]=}")
    print(f"{timeCNRerouting=}")
    print(f"{timeCNUpdate=}")
    print(f"{lossTime=}")
    print(f"{reroutingTime=}")

# FIXME: add asserts
        #assert ((timeSeries <= timeNeighUpdate[MN_i]) | np.isnan(timeNeighUpdate[MN_i])).all(), "Wrong timeordering, check data"
        #assert ((timeNeighUpdate[MN_i] <= timeCNUpdate[MN_i]) | np.isnan(timeNeighUpdate[MN_i]) | np.isnan(timeCNUpdate[MN_i])).all(), "Wrong timeordering, check data"

def first_nonzero(arr, axis, invalid_val=-1):
    mask = arr!=False
    return np.where(mask.any(axis=axis), mask.argmax(axis=axis), invalid_val)

def last_nonzero(arr, axis, invalid_val=-1):
    mask = arr!=False
    # len(axis) - found index - 1
    val = arr.shape[axis] - np.flip(mask, axis=axis).argmax(axis=axis) - 1
    return np.where(mask.any(axis=axis), val, invalid_val)

def argfind(arr, axis=1, side='first'):
    """
    Return indices of first occurence along certain dimension
    Believe it or not, this has not yet been implemented for numpy...
    """
    #corrID_idx, time_idx = np.nonzero(a)
    #corrID_idx, idxidx = np.unique(corrID_idx, return_index=True)
    corrID_idx = np.arange(arr.shape[0])
    if side == 'first':
        time_idx = first_nonzero(arr, axis)
    elif side == 'last':
        time_idx = last_nonzero(arr, axis)
    else:
        raise f"welp, wrong {side=} argument"
    return corrID_idx[time_idx != -1], time_idx[time_idx != -1]


if __name__ == '__main__':
    #              0 1       5         10        15        20
    a = np.array([[0,0,0,0,0,0,0,0,0,1,1,1,0,0,1,1,1,1,1,1,0,0,0,0],
                  [0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],
                  [0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0]])
    def test_argfind():
        corrID_idx, time_idx = argfind(a)
        assert np.all(corrID_idx == np.array([0,1]))
        assert np.all(time_idx == np.array([9,5]))
        corrID_idx, time_idx = argfind(a, side='last')
        assert np.all(corrID_idx == np.array([0,1]))
        assert np.all(time_idx == np.array([19,5]))
        print("test argfind() succeeded")
    def test_first_nonzero():
        time_idx = first_nonzero(a, 1)
        assert np.all(time_idx == np.array([9,5,-1]))
        print("test first_nonzero() succeeded")
    test_argfind()
    test_first_nonzero()
