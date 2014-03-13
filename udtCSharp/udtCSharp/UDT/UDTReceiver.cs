using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Threading;
using udtCSharp.Common;
using udtCSharp.packets;
using udtCSharp.Receiver;

namespace udtCSharp.UDT
{
    /// <summary>
    /// receiver part of a UDT entity
    /// @see UDTSender
    /// </summary>
    public class UDTReceiver 
    {
	    private  UDPEndPoint endpoint;

	    private  UDTSession session;

	    private  UDTStatistics statistics;

	    /// <summary>
	    /// record seqNo of detected lostdata and latest feedback time
	    /// </summary>
	    private ReceiverLossList receiverLossList;

	    //record each sent ACK and the sent time 
	    private AckHistoryWindow ackHistoryWindow;

	    //Packet history window that stores the time interval between the current and the last seq.
	    private PacketHistoryWindow packetHistoryWindow;

	    //for storing the arrival time of the last received data packet
	    private volatile int lastDataPacketArrivalTime=0;

	    //largest received data packet sequence number(LRSN)
	    private volatile int largestReceivedSeqNumber=0;

	    //ACK event related

	    //last Ack number
	    private long lastAckNumber=0;

	    //largest Ack number ever acknowledged by ACK2
	    private volatile int largestAcknowledgedAckNumber=-1;

	    //EXP event related

	    //a variable to record number of continuous EXP time-out events 
	    private volatile int expCount=0;

	    /*records the time interval between each probing pair
        compute the median packet pair interval of the last
	    16 packet pair intervals (PI) and the estimate link capacity.(packet/s)*/
	    private PacketPairWindow packetPairWindow;

	    //estimated link capacity
	    long estimateLinkCapacity;
	    // the packet arrival rate
	    long packetArrivalSpeed;

	    //round trip time, calculated from ACK/ACK2 pairs
	    long roundTripTime=0;
	    //round trip time variance
	    long roundTripTimeVar = 0;

	    //to check the ACK, NAK, or EXP timer
	    private long nextACK;
	    //microseconds to next ACK event
	    private long ackTimerInterval=Util.getSYNTime();

	    private long nextNAK;
	    //microseconds to next NAK event
	    private long nakTimerInterval=Util.getSYNTime();

	    private long nextEXP;
	    //microseconds to next EXP event
	    private long expTimerInterval=100*Util.getSYNTime();

	    //instant when the session was created (for expiry checking)
	    private long sessionUpSince;
	    //milliseconds to timeout a new session that stays idle
	    private long IDLE_TIMEOUT = 3*60*1000;

	    //buffer size for storing data
	    private long bufferSize;

	    //stores received packets to be sent
	    private Queue<UDTPacket> handoffQueue;

	    private Thread receiverThread;

	    private volatile bool stopped=false;

	    //(optional) ack interval (see CongestionControl interface)
	    private volatile int ackInterval=-1;

	    /**
	     * if set to true connections will not expire, but will only be
	     * closed by a Shutdown message
	     */
	    public static bool connectionExpiryDisabled=false;

	    private bool storeStatistics;

        /// <summary>
        /// 注意跟ControlPacket.cs 中的定义ControlPacketType相同
        /// </summary>
        public enum ControlPacketType
        {
            CONNECTION_HANDSHAKE,
            KEEP_ALIVE,
            ACK,
            NAK,
            UNUNSED_1,
            SHUTDOWN,
            ACK2,
            MESSAGE_DROP_REQUEST,
            UNUNSED_2,
            UNUNSED_3,
            UNUNSED_4,
            UNUNSED_5,
            UNUNSED_6,
            UNUNSED_7,
            UNUNSED_8,
            USER_DEFINED
        }

        //( System.DateTime.UtcNow.Ticks - new DateTime(1970, 1, 1, 0, 0, 0).Ticks)/10000;
        //如果要得到Java中 System.currentTimeMillis() 一样的结果，就可以写成 上面那样，也可以这样写：
        // TimeSpan ts=new TimeSpan( System.DateTime.UtcNow.Ticks - new DateTime(1970, 1, 1, 0, 0, 0).Ticks);
        //(long)ts.TotalMilliseconds;
        private TimeSpan ts = new TimeSpan(System.DateTime.UtcNow.Ticks - new DateTime(1970, 1, 1, 0, 0, 0).Ticks);

        /// <summary>
        /// create a receiver with a valid {@link UDTSession}
        /// </summary>
        /// <param name="session"></param>
        /// <param name="endpoint"></param>
	    public UDTReceiver(UDTSession session,UDPEndPoint endpoint)
        {
            this.roundTripTimeVar = roundTripTime / 2;
		    this.endpoint = endpoint;
		    this.session=session;
            //( System.DateTime.UtcNow.Ticks - new DateTime(1970, 1, 1, 0, 0, 0).Ticks)/10000;
            //如果要得到Java中 System.currentTimeMillis() 一样的结果，就可以写成 上面那样，也可以这样写：
            // TimeSpan ts=new TimeSpan( System.DateTime.UtcNow.Ticks - new DateTime(1970, 1, 1, 0, 0, 0).Ticks);
            //(long)ts.TotalMilliseconds;
            this.sessionUpSince = (long)ts.TotalMilliseconds;
		    this.statistics=session.getStatistics();
		    if(!session.isReady())
                Log.Write(this.ToString(),"UDTSession is not ready.");
		    ackHistoryWindow = new AckHistoryWindow(16);
		    packetHistoryWindow = new PacketHistoryWindow(16);
		    receiverLossList = new ReceiverLossList();
		    packetPairWindow = new PacketPairWindow(16);
		    largestReceivedSeqNumber=(int)session.getInitialSequenceNumber() - 1;
		    bufferSize=session.getReceiveBufferSize();
		    handoffQueue=new Queue<UDTPacket>(4*session.getFlowWindowSize());
		    storeStatistics=false;//Boolean.getBoolean("udt.receiver.storeStatistics");
		    initMetrics();
		    start();
	    }
	
	    private MeanValue dgReceiveInterval;
	    private MeanValue dataPacketInterval;
	    private MeanValue processTime;
	    private MeanValue dataProcessTime;
	    private void initMetrics()
        {
		    if(!storeStatistics)return;
		    dgReceiveInterval=new MeanValue("UDT receive interval",false, 64);
		    statistics.addMetric(dgReceiveInterval);
		    dataPacketInterval=new MeanValue("Data packet interval",false, 64);
		    statistics.addMetric(dataPacketInterval);
		    processTime=new MeanValue("UDT packet process time",false, 64);
		    statistics.addMetric(processTime);
		    dataProcessTime=new MeanValue("Data packet process time",false, 64);
		    statistics.addMetric(dataProcessTime);
	    }

	    /// <summary>
	    /// starts the sender algorithm
	    /// </summary>
	    private void start()
        {
            receiverThread = new Thread(new ThreadStart(startThread));
            receiverThread.Start();
	    }
        private void startThread()
        {
            try
            {
                nextACK=Util.getCurrentTime()+ackTimerInterval;
                nextNAK=(long)(Util.getCurrentTime()+1.5*nakTimerInterval);
                nextEXP=Util.getCurrentTime()+2*expTimerInterval;
                ackInterval=(int)session.getCongestionControl().getAckInterval();
                while(!stopped)
                {
                    receiverAlgorithm();
                }
            }
            catch(Exception ex)
            {
                Log.Write(this.ToString(),"SEVERE",ex);
            }
            Log.Write(this.ToString(),"STOPPING RECEIVER for "+session);;
        }
        
        /// <summary>
        /// packets are written by the endpoint
        /// </summary>
        /// <param name="p"></param>
	    public void receive(UDTPacket p)
        {
		    if(storeStatistics)dgReceiveInterval.end();
		    handoffQueue.Enqueue(p);
		    if(storeStatistics)dgReceiveInterval.begin();
	    }

     
        /// <summary>
        /// receiver algorithm 
        /// see specification P11.
        /// </summary>
	    public void receiverAlgorithm()
        {
            try
            {
                if (handoffQueue.Count <= 0)
                {
                    Thread.Yield();
                }
                else
                {
                    //check ACK timer
                    long currentTime = Util.getCurrentTime();

                    #region 暂时,屏蔽不然会一直发包(不知道这段是启什么作用)
                    //if (nextACK < currentTime)
                    //{
                    //    nextACK = currentTime + ackTimerInterval;
                    //    processACKEvent(true);
                    //}
                    ////check NAK timer
                    //if (nextNAK < currentTime)
                    //{
                    //    nextNAK = currentTime + nakTimerInterval;
                    //    processNAKEvent();
                    //}

                    ////check EXP timer
                    //if (nextEXP < currentTime)
                    //{
                    //    nextEXP = currentTime + expTimerInterval;
                    //    processEXPEvent();
                    //}
                    #endregion

                    //perform time-bounded UDP receive
                    UDTPacket packet = handoffQueue.Dequeue();                    
                    if (packet != null)
                    {
                        //reset exp count to 1
                        expCount = 1;
                        //If there is no unacknowledged data packet, or if this is an 
                        //ACK or NAK control packet, reset the EXP timer.
                        bool needEXPReset = false;
                        if (packet.isControlPacket())
                        {
                            ControlPacket cp = (ControlPacket)packet;
                            int cpType = cp.getControlPacketType();
                            if (cpType == (int)ControlPacketType.ACK || cpType == (int)ControlPacketType.NAK)
                            {
                                needEXPReset = true;
                            }
                        }
                        if (needEXPReset)
                        {
                            nextEXP = Util.getCurrentTime() + expTimerInterval;
                        }
                        if (storeStatistics) processTime.begin();
                        //解析数据包将数据存在AppData类中
                        processUDTPacket(packet);

                        if (storeStatistics) processTime.end();
                    }
                    Thread.Yield();
                }
            }
            catch(Exception exc)
            {
                 Log.Write(this.ToString(),exc);
            }
	    }

        /// <summary>
        /// process ACK event (see spec. p 12)
        /// </summary>
        /// <param name="isTriggeredByTimer"></param>
	    protected void processACKEvent(bool isTriggeredByTimer)
        {
            try
            {
		        //(1).Find the sequence number *prior to which* all the packets have been received
		        long ackNumber;
		        ReceiverLossListEntry entry=receiverLossList.getFirstEntry();
		        if (entry==null) {
			        ackNumber = largestReceivedSeqNumber + 1;
		        } else {
			        ackNumber = entry.getSequenceNumber();
		        }
		        //(2).a) if ackNumber equals to the largest sequence number ever acknowledged by ACK2
		        if (ackNumber == largestAcknowledgedAckNumber){
			        //do not send this ACK
			        return;
		        }else if (ackNumber==lastAckNumber) {
			        //or it is equals to the ackNumber in the last ACK  
			        //and the time interval between these two ACK packets
			        //is less than 2 RTTs,do not send(stop)
			        long timeOfLastSentAck=ackHistoryWindow.getTime(lastAckNumber);
			        if(Util.getCurrentTime()-timeOfLastSentAck< 2*roundTripTime){
				        return;
			        }
		        }
		        long ackSeqNumber;
		        //if this ACK is not triggered by ACK timers,send out a light Ack and stop.
		        if(!isTriggeredByTimer){
			        ackSeqNumber=sendLightAcknowledgment(ackNumber);
			        return;
		        }
		        else{
			        //pack the packet speed and link capacity into the ACK packet and send it out.
			        //(7).records  the ACK number,ackseqNumber and the departure time of
			        //this Ack in the ACK History Window
			        ackSeqNumber=sendAcknowledgment(ackNumber);
		        }
		        AckHistoryEntry sentAckNumber= new AckHistoryEntry(ackSeqNumber,ackNumber,Util.getCurrentTime());
		        ackHistoryWindow.add(sentAckNumber);
		        //store ack number for next iteration
		        lastAckNumber=ackNumber;
            }
            catch(Exception exc)
            {
                Log.Write(this.ToString(),exc);
            }
	    }

        /// <summary>
        ///  process NAK event (see spec. p 13)
        /// </summary>
	    protected void processNAKEvent()
        {
            try
            {
		        //find out all sequence numbers whose last feedback time larger than is k*RTT
		        List<long> seqNumbers = receiverLossList.getFilteredSequenceNumbers(roundTripTime,true);
		        sendNAK(seqNumbers);
            }
            catch(Exception exc)
            {
                Log.Write(this.ToString(),exc);
            }
	    }

        /// <summary>
        /// process EXP event (see spec. p 13)
        /// </summary>
	    protected void processEXPEvent()
        {
            try
            {
		        if(session.getSocket()==null)return;
		        UDTSender sender=session.getSocket().getSender();
		        //put all the unacknowledged packets in the senders loss list
		        sender.putUnacknowledgedPacketsIntoLossList();
		        if(expCount>16 && (long)ts.TotalMilliseconds - sessionUpSince > IDLE_TIMEOUT)
                {
			        if(!connectionExpiryDisabled &&!stopped)
                    {
				        sendShutdown();
				        stop();
				        Log.Write(this.ToString(),"Session "+session+" expired.");
				        return;
			        }
		        }
		        if(!sender.haveLostPackets()){
			        sendKeepAlive();
		        }
		        expCount++;
            }
            catch(Exception exc)
            {
                Log.Write(this.ToString(),exc);
            }
	    }

	    protected void processUDTPacket(UDTPacket p)
        {
		    //(3).Check the packet type and process it according to this.
		    try
            {
		        if(!p.isControlPacket())
                {
			        DataPacket dp=(DataPacket)p;
			        if(storeStatistics)
                    {
				        dataPacketInterval.end();
				        dataProcessTime.begin();
			        }
                    onDataPacketReceived(dp);//解析数据包将数据存在AppData类中
			        if(storeStatistics)
                    {
				        dataProcessTime.end();
				        dataPacketInterval.begin();
			        }
		        }

		        else if (p.getControlPacketType()==(int)ControlPacketType.ACK2)
                {
			        Acknowledgment2 ack2=(Acknowledgment2)p;
			        onAck2PacketReceived(ack2);
		        }

		        else if (p is Shutdown){
			        onShutdown();
		        }
            }
            catch(Exception exc)
            {
                Log.Write(this.ToString(),exc);
            }
	    }

	    /// <summary>
	    /// every nth packet will be discarded... for testing only of course
	    /// </summary>
	    public static int dropRate=0;
	
	    /// <summary>
	    /// number of received data packets
	    /// </summary>        
	    private int n=0;
	
	    protected void onDataPacketReceived(DataPacket dp)
        {
            try
            {
		        long currentSequenceNumber = dp.getPacketSequenceNumber();
		
		        //for TESTING : check whether to drop this packet
        //		n++;
        //		//if(dropRate>0 && n % dropRate == 0){
        //			if(n % 1111 == 0){	
        //				logger.info("**** TESTING:::: DROPPING PACKET "+currentSequenceNumber+" FOR TESTING");
        //				return;
        //			}
        //		//}
                bool OK = session.getSocket().getInputStream().haveNewData(currentSequenceNumber, dp.getData());//解析数据包将数据存在AppData类中
		        if(!OK)
                {
			        //need to drop packet...
			        return;
		        }
		
		        long currentDataPacketArrivalTime = Util.getCurrentTime();

		        /*(4).if the seqNo of the current data packet is 16n+1,record the
		        time interval between this packet and the last data packet
		        in the packet pair window*/
		        if((currentSequenceNumber%16)==1 && lastDataPacketArrivalTime>0){
			        long interval=currentDataPacketArrivalTime -lastDataPacketArrivalTime;
			        packetPairWindow.add(interval);
		        }
		
		        //(5).record the packet arrival time in the PKT History Window.
		        packetHistoryWindow.add(currentDataPacketArrivalTime);

		
		        //store current time
		        lastDataPacketArrivalTime=(int)currentDataPacketArrivalTime;

		
		        //(6).number of detected lossed packet
		        /*(6.a).if the number of the current data packet is greater than LSRN+1,
			        put all the sequence numbers between (but excluding) these two values
			        into the receiver's loss list and send them to the sender in an NAK packet*/
		        if(SequenceNumber.compare(currentSequenceNumber,largestReceivedSeqNumber+1)>0){
			        sendNAK(currentSequenceNumber);
		        }
		        else if(SequenceNumber.compare(currentSequenceNumber,largestReceivedSeqNumber)<0){
				        /*(6.b).if the sequence number is less than LRSN,remove it from
				         * the receiver's loss list
				         */
				        receiverLossList.remove(currentSequenceNumber);
		        }

		        statistics.incNumberOfReceivedDataPackets();

		        //(7).Update the LRSN
		        if(SequenceNumber.compare(currentSequenceNumber,largestReceivedSeqNumber)>0){
                    largestReceivedSeqNumber = (int)currentSequenceNumber;
		        }

		        //(8) need to send an ACK? Some cc algorithms use this
		        if(ackInterval>0)
                {
			        if(n % ackInterval == 0)processACKEvent(false);
		        }
            }
            catch(Exception exc)
            {
                Log.Write(this.ToString(),exc);
            }
	    }

	    /**
	     * write a NAK triggered by a received sequence number that is larger than
	     * the largestReceivedSeqNumber + 1
	     * @param currentSequenceNumber - the currently received sequence number
	     * @throws IOException
	     */
	    protected void sendNAK(long currentSequenceNumber)
        {
            try
            {
		        NegativeAcknowledgement nAckPacket= new NegativeAcknowledgement();
		        nAckPacket.addLossInfo(largestReceivedSeqNumber+1, currentSequenceNumber);
		        nAckPacket.setSession(session);
		        nAckPacket.setDestinationID(session.getDestination().getSocketID());
		        //put all the sequence numbers between (but excluding) these two values into the
		        //receiver loss list
		        for(long i=largestReceivedSeqNumber+1;i<currentSequenceNumber;i++){
			        ReceiverLossListEntry detectedLossSeqNumber= new ReceiverLossListEntry(i);
			        receiverLossList.insert(detectedLossSeqNumber);
		        }
		        endpoint.doSend(nAckPacket);
		        //logger.info("NAK for "+currentSequenceNumber);
		        statistics.incNumberOfNAKSent();
             }
             catch(Exception exc)
             {
                 Log.Write(this.ToString(),exc);
             }
	    }

	    protected void sendNAK(List<long>sequenceNumbers)
        {
            try
            {
		        if(sequenceNumbers.Count==0)return;
		        NegativeAcknowledgement nAckPacket= new NegativeAcknowledgement();
		        nAckPacket.addLossInfo(sequenceNumbers);
		        nAckPacket.setSession(session);
		        nAckPacket.setDestinationID(session.getDestination().getSocketID());
		        endpoint.doSend(nAckPacket);
		        statistics.incNumberOfNAKSent();
            }
            catch(Exception exc)
            {
                Log.Write(this.ToString(),exc);
            }
	    }

	    protected long sendLightAcknowledgment(long ackNumber)
        {
            try
            {
		        Acknowledgement acknowledgmentPkt=buildLightAcknowledgement(ackNumber);
		        endpoint.doSend(acknowledgmentPkt);
		        statistics.incNumberOfACKSent();
		        return acknowledgmentPkt.getAckSequenceNumber();
            }
            catch(Exception exc)
            {
                Log.Write(this.ToString(),exc);
                return 0;
            }
	    }

	    protected long sendAcknowledgment(long ackNumber)
        {
            try
            {
		        Acknowledgement acknowledgmentPkt = buildLightAcknowledgement(ackNumber);
		        //set the estimate link capacity
		        estimateLinkCapacity=packetPairWindow.getEstimatedLinkCapacity();
		        acknowledgmentPkt.setEstimatedLinkCapacity(estimateLinkCapacity);
		        //set the packet arrival rate
		        packetArrivalSpeed=packetHistoryWindow.getPacketArrivalSpeed();
		        acknowledgmentPkt.setPacketReceiveRate(packetArrivalSpeed);

		        endpoint.doSend(acknowledgmentPkt);

		        statistics.incNumberOfACKSent();
		        statistics.setPacketArrivalRate(packetArrivalSpeed, estimateLinkCapacity);
		        return acknowledgmentPkt.getAckSequenceNumber();
            }
            catch(Exception exc)
            {
                Log.Write(this.ToString(),exc);
                return 0;
            }
	    }

	    /// <summary>
	    /// builds a "light" Acknowledgement
	    /// </summary>
	    /// <param name="ackNumber"></param>
	    /// <returns></returns>
	    private Acknowledgement buildLightAcknowledgement(long ackNumber)
        {
		    Acknowledgement acknowledgmentPkt = new Acknowledgement();
		    //the packet sequence number to which all the packets have been received
		    acknowledgmentPkt.setAckNumber(ackNumber);
		    //assign this ack a unique increasing ACK sequence number
		    acknowledgmentPkt.setAckSequenceNumber(++ackSequenceNumber);
		    acknowledgmentPkt.setRoundTripTime(roundTripTime);
		    acknowledgmentPkt.setRoundTripTimeVar(roundTripTimeVar);
		    //set the buffer size
		    acknowledgmentPkt.setBufferSize(bufferSize);

		    acknowledgmentPkt.setDestinationID(session.getDestination().getSocketID());
		    acknowledgmentPkt.setSession(session);

		    return acknowledgmentPkt;
	    }

	    /**
	     * spec p. 13: <br/>
	      1) Locate the related ACK in the ACK History Window according to the 
             ACK sequence number in this ACK2.  <br/>
          2) Update the largest ACK number ever been acknowledged. <br/>
          3) Calculate new rtt according to the ACK2 arrival time and the ACK 
             departure time, and update the RTT value as: RTT = (RTT * 7 + 
             rtt) / 8.  <br/>
          4) Update RTTVar by: RTTVar = (RTTVar * 3 + abs(RTT - rtt)) / 4.  <br/>
          5) Update both ACK and NAK period to 4 * RTT + RTTVar + SYN.  <br/>
	     */
	    protected void onAck2PacketReceived(Acknowledgment2 ack2)
        {
		    AckHistoryEntry entry=ackHistoryWindow.getEntry(ack2.getAckSequenceNumber());
		    if(entry!=null)
            {
			    long ackNumber=entry.getAckNumber();
                largestAcknowledgedAckNumber = (int)Math.Max(ackNumber, largestAcknowledgedAckNumber);
			
			    long rtt=entry.getAge();
			    if(roundTripTime>0)roundTripTime = (roundTripTime*7 + rtt)/8;
			    else roundTripTime = rtt;
			    roundTripTimeVar = (roundTripTimeVar* 3 + Math.Abs(roundTripTimeVar - rtt)) / 4;
			    ackTimerInterval=4*roundTripTime+roundTripTimeVar+Util.getSYNTime();
			    nakTimerInterval=ackTimerInterval;
			    statistics.setRTT(roundTripTime, roundTripTimeVar);
		    }
	    }

	    protected void sendKeepAlive()
        {
            try
            {
		        KeepAlive ka=new KeepAlive();
		        ka.setDestinationID(session.getDestination().getSocketID());
		        ka.setSession(session);
		        endpoint.doSend(ka);
            }
            catch(Exception exc)
            {
                Log.Write(this.ToString(),exc);
            }
	    }

	    protected void sendShutdown()
        {
            try
            {
		        Shutdown s=new Shutdown();
		        s.setDestinationID(session.getDestination().getSocketID());
		        s.setSession(session);
		        endpoint.doSend(s);
             }
            catch(Exception exc)
            {
                Log.Write(this.ToString(),exc);
            }
	    }

	    private volatile int ackSequenceNumber=0;

	    public void resetEXPTimer()
        {
		    nextEXP=Util.getCurrentTime()+expTimerInterval;
		    expCount=0;
	    }

	    public void resetEXPCount()
        {
		    expCount=0;
	    }
	
	    public void setAckInterval(long ackInterval){
		    this.ackInterval=(int)ackInterval;
	    }
	
	    protected void onShutdown()
        {
            try
            {
		        stop();
            }
            catch(Exception exc)
            {
                Log.Write(this.ToString(),exc);
            }
	    }

	    public void stop()
        {
            try
            {
		        stopped=true;
		        session.getSocket().close();
		        //stop our sender as well
		        session.getSocket().getSender().stop();
            }
            catch (Exception exc)
            {
                Log.Write(this.ToString(), exc);
            }
	    }

	    public String toString()
        {
		    StringBuilder sb=new StringBuilder();
		    sb.Append("UDTReceiver ").Append(session).Append("\n");
		    sb.Append("LossList: "+receiverLossList);
		    return sb.ToString();
	    }

    }

}
