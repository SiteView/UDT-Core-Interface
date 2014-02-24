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

        private volatile int roundTripTime;
        private volatile int roundTripTimeVariance;
        private volatile int packetArrivalRate;
        private volatile int estimatedLinkCapacity;
        private volatile float sendPeriod;
        private volatile int congestionWindowSize;

        public UDTStatistics(String componentDescription)
        {
            this.componentDescription = componentDescription;
        }

        public void setSendPeriod(double sendPeriod)
        {
            this.sendPeriod = (float)sendPeriod;
        }
    }
}
