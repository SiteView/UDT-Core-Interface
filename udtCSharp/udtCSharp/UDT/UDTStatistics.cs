using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace udtCSharp.UDT
{
    public class UDTStatistics
    {
        private String componentDescription;

	    private volatile long roundTripTime;
	    private volatile long roundTripTimeVariance;
	    private volatile long packetArrivalRate;
	    private volatile long estimatedLinkCapacity;
	    private volatile double sendPeriod;
	    private volatile long congestionWindowSize;

        public UDTStatistics(String componentDescription)
        {
            this.componentDescription = componentDescription;
        }
    }
}
