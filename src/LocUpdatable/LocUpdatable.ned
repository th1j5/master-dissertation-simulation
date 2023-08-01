package prototype.LocUpdatable;

moduleinterface ILocUpdatable
{
    parameters:
        @signal[neighLocatorUpdateSent](type=inet::Packet);
        @signal[oldLocatorUnreachable](type=int); // int numLocUpdates
        @signal[newLocatorAssigned](type=int); // int numLocUpdates
        @statistic[oldLocatorUnreachable](title="old locator becomes unreachable";record=vector(locUpdateCorrelation); interpolationmode=none;);
        @statistic[newLocatorAssigned](title="new locator assigned";record=vector(locUpdateCorrelation); interpolationmode=none;);
}
