import math
import numpy as np
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
############### experiment ###########
    # 1 row voor elke MN
    sendRows = df.loc[df["name"].str.contains("newLocatorAssigned:vector")].reset_index(drop=True)
    locRemoveRows = df.loc[df["name"].str.contains("oldLocatorUnreachable:vector")].reset_index(drop=True)
    neighRows = df.loc[df["name"].str.contains("neighLocatorUpdateReceived:vector")].reset_index(drop=True)
    # meerdere rows voor elke server per client
    rcvdRows = df.loc[df["name"].str.contains("locatorUpdateReceived:vector")].reset_index(drop=True)
    end2endDelayRows = df.loc[df["name"].str.contains("endToEndDelay:vector")].reset_index(drop=True)

    sendRows[["vectime","vecvalue"]]
    time = sendRows["vectime"]
    # dimensions
    numMN = sendRows.shape[0] # Mobile Nodes
    numLocUpdates = sendRows["vectime"].apply(np.size) # Series
    numCN = np.nan # Corresponding Nodes (for each LocUpdate)
    # data structures
    # FIXME: assumption: only 1 neigh or CN
    dim = (numMN, np.max(numLocUpdates))
    timeOldLocRemove = np.empty(dim)
    timeCNUpdate = np.empty(dim)
    timeNeighUpdate = np.empty(dim)
    lossTime = np.empty(dim)
    reroutingTime = np.empty(dim)
    delay = np.empty((3, *dim)) # before, during, after
    timeOldLocRemove[:] = np.nan
    timeCNUpdate[:] = np.nan
    timeNeighUpdate[:] = np.nan
    lossTime[:] = np.nan
    reroutingTime[:] = np.nan
    delay[:] = np.nan

    # slow
    for MN_i, timeSeries in enumerate(time.to_numpy()):
        for i, t in enumerate(timeSeries):
            corrID = sendRows["vecvalue"][MN_i][i]
            ## locRemove
            row = locRemoveRows.iloc[0]
            times = row["vectime"][row["vecvalue"] == corrID] # could be multiple
            if len(times) != 1:
                print(f"Warning: oldLocatorUnreachable has {len(times)=}, this probably means that it was unreachable for an amount of time")
                print(f"Check correct behaviour of this case")
            timeOldLocRemove[MN_i,i] = times[0] # should be earliest
            ## CN
            #for server, row in rcvdRows.iterrows():
            row = rcvdRows.iloc[0]
            # each server got exactly 1 update
            timeCNUpdate[MN_i,i] = row["vectime"][row["vecvalue"] == corrID]
            ## neigh
            for _, row in neighRows.iterrows():
                neighTime = row["vectime"][row["vecvalue"] == corrID]
                if neighTime.size != 0:
                    timeNeighUpdate[MN_i, i] = neighTime
        assert ((timeSeries <= timeNeighUpdate[MN_i]) | np.isnan(timeNeighUpdate[MN_i])).all(), "Wrong timeordering, check data"
        assert ((timeNeighUpdate[MN_i] <= timeCNUpdate[MN_i]) | np.isnan(timeNeighUpdate[MN_i]) | np.isnan(timeCNUpdate[MN_i])).all(), "Wrong timeordering, check data"
    print(time.to_numpy())
    print(timeOldLocRemove)
    print(timeNeighUpdate)
    print(timeCNUpdate)
    assert sendRows["vecvalue"].equals(rcvdRows["vecvalue"]), "assume only 1 MN and 1 CN"
    
#locatorUpdateSent
    for MN_i, timeSeries in enumerate(time.to_numpy()):
        row = end2endDelayRows.iloc[0]
        # https://stackoverflow.com/a/26026189
        filterDelayBeforeIdx = np.searchsorted(row["vectime"], timeOldLocRemove[MN_i, :], side="right") -1 # TODO check side parameter
        # don't assign when negative idx
        delay[0,MN_i][filterDelayBeforeIdx>=0] = row["vecvalue"][filterDelayBeforeIdx][filterDelayBeforeIdx >= 0]
        print(filterDelayBeforeIdx)
        print(row["vectime"][filterDelayBeforeIdx][filterDelayBeforeIdx>=0])
    print(delay)
#    lossTime =
#    reroutingTime = 

############################
    
    # apply vector operations
    df = utils.perform_vector_ops(df, props["vector_operations"])
    return df


if __name__ == '__main__':
    print("TODO: test module")
