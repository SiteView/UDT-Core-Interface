using System;
using System.Collections.Generic;
using System.Collections;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Threading;
using System.IO;
using udtCSharp.Common;

namespace udtCSharp.UDT
{
    /// <summary>
    /// This class is used to keep some statistics about a UDT connection. 
    /// </summary>
    public class UDTStatistics 
    {

	    private  int numberOfSentDataPackets = 0;
	    private  int numberOfReceivedDataPackets=0;
	    private  int numberOfDuplicateDataPackets=0;
	    private  int numberOfMissingDataEvents= 0;
	    private  int numberOfNAKSent= 0;
	    private  int numberOfNAKReceived= 0;
	    private  int numberOfRetransmittedDataPackets= 0;
	    private  int numberOfACKSent= 0;
	    private  int numberOfACKReceived= 0;
	    private  int numberOfCCSlowDownEvents= 0;
	    private  int numberOfCCWindowExceededEvents= 0;

	    private String componentDescription;

	    private volatile long roundTripTime;
	    private volatile long roundTripTimeVariance;
	    private volatile long packetArrivalRate;
	    private volatile long estimatedLinkCapacity;
	    private volatile double sendPeriod;
	    private volatile long congestionWindowSize;
        
	    private List<MeanValue>metrics=new List<MeanValue>();
		
	    public UDTStatistics(String componentDescription){
		    this.componentDescription=componentDescription;
	    }

	    public int getNumberOfSentDataPackets() 
        {
		    return numberOfSentDataPackets;
	    }
	    public int getNumberOfReceivedDataPackets() 
        {
		    return numberOfReceivedDataPackets;
	    }
	    public int getNumberOfDuplicateDataPackets() 
        {
		    return numberOfDuplicateDataPackets;
	    }
	    public int getNumberOfNAKSent() 
        {
		    return numberOfNAKSent;
	    }
	    public int getNumberOfNAKReceived() 
        {
		    return numberOfNAKReceived;
	    }
	    public int getNumberOfRetransmittedDataPackets() 
        {
		    return numberOfRetransmittedDataPackets;
	    }
	    public int getNumberOfACKSent() 
        {
		    return numberOfACKSent;
	    }
	    public int getNumberOfACKReceived() 
        {
		    return numberOfACKReceived;
	    }
	    public void incNumberOfSentDataPackets() 
        {
		    Interlocked.Increment(ref numberOfSentDataPackets);
	    }
	    public void incNumberOfReceivedDataPackets() 
        {
		    Interlocked.Increment(ref numberOfReceivedDataPackets);
	    }
	    public void incNumberOfDuplicateDataPackets()
        {
		    Interlocked.Increment(ref numberOfDuplicateDataPackets);
	    }
	    public void incNumberOfMissingDataEvents() 
        {
		    Interlocked.Increment(ref numberOfMissingDataEvents);
	    }
	    public void incNumberOfNAKSent() 
        {
		    Interlocked.Increment(ref numberOfNAKSent);
	    }
	    public void incNumberOfNAKReceived() 
        {
		   Interlocked.Increment(ref numberOfNAKReceived);
	    }
	    public void incNumberOfRetransmittedDataPackets() 
        {
		    Interlocked.Increment(ref numberOfRetransmittedDataPackets);
	    }

	    public void incNumberOfACKSent() 
        {
		   Interlocked.Increment(ref numberOfACKSent);
	    }

	    public void incNumberOfACKReceived() {
		    Interlocked.Increment(ref numberOfACKReceived);
	    }

	    public void incNumberOfCCWindowExceededEvents() {
		    Interlocked.Increment(ref numberOfCCWindowExceededEvents);
	    }

	    public void incNumberOfCCSlowDownEvents() {
		    Interlocked.Increment(ref numberOfCCSlowDownEvents);
	    }

	    public void setRTT(long rtt, long rttVar){
		    this.roundTripTime=rtt;
		    this.roundTripTimeVariance=rttVar;
	    }

	    public void setPacketArrivalRate(long rate, long linkCapacity){
		    this.packetArrivalRate=rate;
		    this.estimatedLinkCapacity=linkCapacity;
	    }

	    public void setSendPeriod(double sendPeriod){
		    this.sendPeriod=sendPeriod;
	    }

	    public double getSendPeriod(){
		    return sendPeriod;
	    }

	    public long getCongestionWindowSize() {
		    return congestionWindowSize;
	    }

	    public void setCongestionWindowSize(long congestionWindowSize) {
		    this.congestionWindowSize = congestionWindowSize;
	    }

	    public long getPacketArrivalRate(){
		    return packetArrivalRate;
	    }

	    /**
	     * add a metric
	     * @param m - the metric to add
	     */
	    public void addMetric(MeanValue m){
		    metrics.Add(m);
	    }
	
	    /**
	     * get a read-only list containing all metrics
	     * @return
	     */
	    public List<MeanValue>getMetrics()
        {
		    return metrics;
	    }
	
	    public String toString(){
		    StringBuilder sb=new StringBuilder();
		    sb.Append("Statistics for ").Append(componentDescription).Append("\n");
		    sb.Append("Sent data packets: ").Append(getNumberOfSentDataPackets()).Append("\n");
		    sb.Append("Received data packets: ").Append(getNumberOfReceivedDataPackets()).Append("\n");
		    sb.Append("Duplicate data packets: ").Append(getNumberOfDuplicateDataPackets()).Append("\n");
		    sb.Append("ACK received: ").Append(getNumberOfACKReceived()).Append("\n");
		    sb.Append("NAK received: ").Append(getNumberOfNAKReceived()).Append("\n");
		    sb.Append("Retransmitted data: ").Append(getNumberOfNAKReceived()).Append("\n");
		    sb.Append("NAK sent: ").Append(getNumberOfNAKSent()).Append("\n");
		    sb.Append("ACK sent: ").Append(getNumberOfACKSent()).Append("\n");
		    if(roundTripTime>0){
			    sb.Append("RTT ").Append(roundTripTime).Append(" var. ").Append(roundTripTimeVariance).Append("\n");
		    }
		    if(packetArrivalRate>0){
			    sb.Append("Packet rate: ").Append(packetArrivalRate).Append("/sec., link capacity: ").Append(estimatedLinkCapacity).Append("/sec.\n");
		    }
		    if(numberOfMissingDataEvents>0){
			    sb.Append("Sender without data events: ").Append(numberOfMissingDataEvents).Append("\n");
		    }
		    if(numberOfCCSlowDownEvents>0){
			    sb.Append("CC rate slowdown events: ").Append(numberOfCCSlowDownEvents).Append("\n");
		    }
		    if(numberOfCCWindowExceededEvents>0){
			    sb.Append("CC window slowdown events: ").Append(numberOfCCWindowExceededEvents).Append("\n");
		    }
		    sb.Append("CC parameter SND:  ").Append((int)sendPeriod).Append("\n");
		    sb.Append("CC parameter CWND: ").Append(congestionWindowSize).Append("\n");
		    foreach(MeanValue v in metrics)
            {
			    sb.Append(v.getName()).Append(": ").Append(v.getFormattedMean()).Append("\n");
		    }
		    return sb.ToString();
	    }

	    private  List<StatisticsHistoryEntry> statsHistory = new List<StatisticsHistoryEntry>();
	    bool first=true;
	    private long initialTime;
	    /**
	     * take a snapshot of relevant parameters for later storing to
	     * file using {@link #writeParameterHistory(File)}
	     */
	    public void storeParameters()
        {
		    lock (statsHistory) 
            {
			    if(first)
                {
				    first=false;
				    statsHistory.Add(new StatisticsHistoryEntry(true,0,metrics));
				    initialTime=System.DateTime.Now.Millisecond;
			    }
                statsHistory.Add(new StatisticsHistoryEntry(false, System.DateTime.Now.Millisecond - initialTime, metrics));
		    }
	    }

        ///**
        // * write saved parameters to disk 
        // * @param toFile
        // */
        /// <summary>
        /// write saved parameters to disk 
        /// </summary>
        /// <param name="toFile">文件名称</param>
	    public void writeParameterHistory(string toFile)
        {
            
            StreamWriter fos = new StreamWriter(toFile);
		    try
            {
			    lock (statsHistory) 
                {
				    foreach(StatisticsHistoryEntry s in statsHistory)
                    {
					    fos.WriteLine(s.toString());
				    }
			    }
		    }
            finally
            {
			    fos.Close();
		    }
	    }

    }
}
