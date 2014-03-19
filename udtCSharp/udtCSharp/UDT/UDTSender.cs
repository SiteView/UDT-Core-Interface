using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Threading;
using udtCSharp.Common;
using udtCSharp.packets;
using udtCSharp.Receiver;
using udtCSharp.Sender;

namespace udtCSharp.UDT
{
    /// <summary>
    /// sender part of a UDT entity 
    /// @see UDTReceiver
    /// </summary>
    public class UDTSender 
    {
	    private UDPEndPoint endpoint;

	    private UDTSession session;

	    private UDTStatistics statistics;

        /// <summary>
        /// senderLossList stores the sequence numbers of lost packets
        /// fed back by the receiver through NAK pakets
        /// </summary>
	    private SenderLossList senderLossList;
	
        /// <summary>
        /// sendBuffer stores the sent data packets and their sequence numbers
        /// </summary>
	    private Dictionary<long,DataPacket> sendBuffer;

        /// <summary>
        /// sendQueue contains the packets to send
        /// </summary>
	    private Queue<DataPacket> sendQueue;

        /// <summary>
        /// 用于控制队列的发送
        /// </summary>
        private volatile CountDownLatch sendQueueStart = new CountDownLatch(1);

        /// <summary>
        /// thread reading packets from send queue and sending them
        /// </summary>
	    private Thread senderThread;

        /// <summary>
        /// protects against races when reading/writing to the sendBuffer
        /// </summary>
	    private object sendLock=new object();

        /// <summary>
        /// number of unacknowledged data packets
        /// </summary>
	    private AtomicInteger unacknowledged = new AtomicInteger(0);

        /// <summary>
        /// for generating data packet sequence numbers
        /// </summary>
	    private volatile int currentSequenceNumber=0;

	    /// <summary>
	    /// the largest data packet sequence number that has actually been sent out
	    /// </summary>
	    private volatile int largestSentSequenceNumber=-1;

	    //last acknowledge number, initialised to the initial sequence number
	    private volatile int lastAckSequenceNumber;

	    private volatile bool started=false;

	    private volatile bool stopped=false;

	    private volatile bool paused=false;

	    /// <summary>
        /// 用于启动发送者发送信号
	    /// </summary>
	    private volatile CountDownLatch startLatch=new CountDownLatch(1);

	    //used by the sender to wait for an ACK
	    private AtomicReference<CountDownLatch> waitForAckLatch=new AtomicReference<CountDownLatch>();

	    //used by the sender to wait for an ACK of a certain sequence number
	    private AtomicReference<CountDownLatch> waitForSeqAckLatch=new AtomicReference<CountDownLatch>();

	    private bool storeStatistics;

	    public UDTSender(UDTSession session,UDPEndPoint endpoint)
        {
		    if(!session.isReady())
                Log.Write(this.ToString(),"UDTSession is not ready.");
		    this.endpoint= endpoint;
		    this.session=session;
		    statistics=session.getStatistics();
		    senderLossList=new SenderLossList();
            sendBuffer = new Dictionary<long, DataPacket>(session.getFlowWindowSize());
            sendQueue = new Queue<DataPacket>(1000);
            lastAckSequenceNumber = (int)session.getInitialSequenceNumber();
            currentSequenceNumber = (int)session.getInitialSequenceNumber() - 1;
		    waitForAckLatch.Set(new CountDownLatch(1));
		    waitForSeqAckLatch.Set(new CountDownLatch(1));
		    storeStatistics= false;//Boolean.getBoolean("udt.sender.storeStatistics");
		    initMetrics();
		    doStart();
	    }

	    private MeanValue dgSendTime;
	    private MeanValue dgSendInterval;
	    private MeanThroughput throughput;

	    private void initMetrics()
        {
		    if(!storeStatistics)return;
		    dgSendTime=new MeanValue("Datagram send time");
		    statistics.addMetric(dgSendTime);
		    dgSendInterval=new MeanValue("Datagram send interval");
		    statistics.addMetric(dgSendInterval);
		    throughput=new MeanThroughput("Throughput", session.getDatagramSize());
		    statistics.addMetric(throughput);
	    }

        /// <summary>
        /// start the sender thread
        /// </summary>
	    public void start()
        {
		    Log.Write(this.ToString(),"Starting sender for "+session);
		    startLatch.CountDown();
		    started=true;
	    }

        /// <summary>
        /// starts the sender algorithm
        /// </summary>
	    private void doStart()
        {
            ThreadStart threadDelegate = new ThreadStart(doStartRun);
            senderThread = UDTThreadFactory.get().newThread(threadDelegate);
            senderThread.Start();
	    }
         public void doStartRun()
         {
			try
            {
				while(!stopped)
                {
					//wait until explicitely (re)started  等待明确通知后在启动。
					startLatch.Await();
					paused=false;
					senderAlgorithm();
				}
			}
            catch(ThreadInterruptedException ie)
            {
                Log.Write(this.ToString(),"SEVERE",ie);				
			}
			catch(Exception ex)
            {				
				Log.Write(this.ToString(),"SEVERE",ex);
			}
			Log.Write(this.ToString(),"STOPPING SENDER for "+ session);
		}
      
         /// <summary>
         /// sends the given data packet, storing the relevant information
         /// 发送存储相关信息的数据包
         /// </summary>
         /// <param name="p"></param>
	    private void send(DataPacket p)
        {
		    lock(sendLock)
            {
			    if(storeStatistics)
                {
				    dgSendInterval.end();
				    dgSendTime.begin();
			    }
			    endpoint.doSend(p);
			    if(storeStatistics)
                {
				    dgSendTime.end();
				    dgSendInterval.begin();
				    throughput.end();
				    throughput.begin();
			    }
                sendBuffer[p.getPacketSequenceNumber()] = p;			    
			    unacknowledged.IncrementAndGet();
		    }
		    statistics.incNumberOfSentDataPackets();
	    }       
     
        /// <summary>
        /// writes a data packet into the sendQueue, waiting at most for the specified time
        /// if this is not possible due to a full send queue
        /// </summary>
        /// <param name="p"></param>
        /// <param name="timeout"></param>
        /// <param name="units"></param>
        /// <returns>@return <code>true</code>if the packet was added, <code>false</code> if the packet could not be added because the queue was full</returns>
        //protected boolean sendUdtPacket(DataPacket p, int timeout, TimeUnit units)//java
        //public bool sendUdtPacket(DataPacket p)
        public bool sendUdtPacket(DataPacket p, int timeout)
        {
            try
            {
                sendQueue.Enqueue(p);

                if (!started) start();

                return true;
            }
            catch(Exception ex)
            {                
                Log.Write(this.ToString(),ex);
                return false;
            }
	    }

	    /// <summary>
	    /// receive a packet from server from the peer
        /// 从对等服务器接收数据包
	    /// </summary>
	    /// <param name="p"></param>
	    public void receive(UDTPacket p)
        {
            try
            {
		        if (p is Acknowledgement) 
                {
			        Acknowledgement acknowledgement=(Acknowledgement)p;
			        onAcknowledge(acknowledgement);
		        }
		        else if (p is NegativeAcknowledgement)
                {
			        NegativeAcknowledgement nak=(NegativeAcknowledgement)p;
			        onNAKPacketReceived(nak);
		        }
		        else if (p is KeepAlive)
                {
			        session.getSocket().getReceiver().resetEXPCount();
		        }
                else if (p is DataPacket)
                {
                    ////返回的确认数据包
                    ////DataPacketAnswer Answer = (DataPacketAnswer)p;
                    //if (largestSentSequenceNumber == (int)p.getPacketSequenceNumber())
                    //{
                    //    sendQueueStart.CountDown();
                    //}
                }
            }
            catch(Exception ex)
            {
                Log.Write(this.ToString(),ex);
            }
	    }

	    protected void onAcknowledge(Acknowledgement acknowledgement)
        {
            try
            {
		        waitForAckLatch.Get().CountDown();
		        waitForSeqAckLatch.Get().CountDown();

		        CongestionControl cc=session.getCongestionControl();
		        long rtt=acknowledgement.getRoundTripTime();
		        if(rtt>0){
			        long rttVar=acknowledgement.getRoundTripTimeVar();
			        cc.setRTT(rtt,rttVar);
			        statistics.setRTT(rtt, rttVar);
		        }
		        long rate=acknowledgement.getPacketReceiveRate();
		        if(rate>0){
			        long linkCapacity=acknowledgement.getEstimatedLinkCapacity();
			        cc.updatePacketArrivalRate(rate, linkCapacity);
			        statistics.setPacketArrivalRate(cc.getPacketArrivalRate(), cc.getEstimatedLinkCapacity());
		        }

		        long ackNumber=acknowledgement.getAckNumber();
		        cc.onACK(ackNumber);
		        statistics.setCongestionWindowSize((long)cc.getCongestionWindowSize());
		        //need to remove all sequence numbers up the ack number from the sendBuffer
		        bool removed=false;
		        for(long s=lastAckSequenceNumber;s<ackNumber;s++)
                {
			        lock (sendLock) 
                    {
				        removed= sendBuffer.Remove(s);
			        }
			        if(removed)
                    {
				        unacknowledged.DecrementAndGet();
			        }
		        }
		        lastAckSequenceNumber=(int)Math.Max(lastAckSequenceNumber, ackNumber);
		        //send ACK2 packet to the receiver
		        sendAck2(ackNumber);
		        statistics.incNumberOfACKReceived();
		        if(storeStatistics)statistics.storeParameters();
            }
            catch(Exception ex)
            {
                Log.Write(this.ToString(),ex);
            }
	    }
   
        /// <summary>
        /// procedure when a NAK is received (spec. p 14)
        /// </summary>
        /// <param name="nak"></param>
	    protected void onNAKPacketReceived(NegativeAcknowledgement nak)
        {
             try
            {
		        foreach(int i in nak.getDecodedLossInfo())
                {
			        senderLossList.insert((long)i);
		        }
		        session.getCongestionControl().onLoss(nak.getDecodedLossInfo());
		        session.getSocket().getReceiver().resetEXPTimer();
		        statistics.incNumberOfNAKReceived();	
		        
			    Log.Write(this.ToString(),"NAK for "+nak.getDecodedLossInfo().Count+" packets lost, " 
					    +"set send period to "+session.getCongestionControl().getSendInterval());
            }
            catch(Exception ex)
            {
                Log.Write(this.ToString(),ex);
            }
		    
	    }

	    /// <summary>
	    /// send single keep alive packet -> move to socket!
        /// 发送单个活动包 -> 到socket中
	    /// </summary>
	    protected void sendKeepAlive()
        {
            try
            {
		        KeepAlive keepAlive = new KeepAlive();
		        //TODO
		        keepAlive.setSession(session);
		        endpoint.doSend(keepAlive);
            }
            catch(Exception ex)
            {
                Log.Write(this.ToString(),ex);
            }
	    }

	    protected void sendAck2(long ackSequenceNumber)
        {
             try
            {
		        Acknowledgment2 ackOfAckPkt = new Acknowledgment2();
		        ackOfAckPkt.setAckSequenceNumber(ackSequenceNumber);
		        ackOfAckPkt.setSession(session);
		        ackOfAckPkt.setDestinationID(session.getDestination().getSocketID());
		        endpoint.doSend(ackOfAckPkt);
            }
            catch(Exception ex)
            {
                Log.Write(this.ToString(),ex);
            }
	    }

	 
	    long iterationStart;
        /// <summary>
        /// 发送算法
        /// </summary>
	    public void senderAlgorithm()
        {
            while (!paused)
            {
                if (this.stopped) return;
                if (sendQueue.Count <= 0)
                {
                    Thread.Sleep(50);
                    continue;
                }

                //iterationStart=Util.getCurrentTime();

                ////if the sender's loss list is not empty 
                //if (!senderLossList.isEmpty()) 
                //{
                //    long entry=senderLossList.getFirstEntry();
                //    handleResubmit(entry);
                //}
                //else
                //{
                //    //if the number of unacknowledged data packets does not exceed the congestion 
                //    //and the flow window sizes, pack a new packet
                //    int unAcknowledged=unacknowledged.Get();

                //    if(unAcknowledged<session.getCongestionControl().getCongestionWindowSize() && unAcknowledged<session.getFlowWindowSize())
                //    {
                //        //check for application data
                //        //DataPacket dp=sendQueue.poll(Util.SYN,TimeUnit.MICROSECONDS);
                //        DataPacket dp = sendQueue.Dequeue();
                //        if(dp!=null)
                //        {
                //            Log.Write(this.ToString(), "发送数据 PacketSequenceNumber =" + dp.getPacketSequenceNumber() + "  数据长度：" + dp.getData().Length);
                //            send(dp);
                //            largestSentSequenceNumber = (int)dp.getPacketSequenceNumber();

                //        }
                //        else
                //        {
                //            statistics.incNumberOfMissingDataEvents();
                //        }
                //    }
                //    else
                //    {
                //        //congestion window full, wait for an ack
                //        if(unAcknowledged>=session.getCongestionControl().getCongestionWindowSize())
                //        {
                //            statistics.incNumberOfCCWindowExceededEvents();
                //        }
                //        waitForAck();
                //    }
                //}

                ////wait
                //if(largestSentSequenceNumber % 16 !=0)
                //{
                //    long snd=(long)session.getCongestionControl().getSendInterval();
                //    long passed=Util.getCurrentTime()-iterationStart;
                //    int x=0;
                //    while(snd-passed>0)
                //    {
                //        //can't wait with microsecond precision :(
                //        if(x==0)
                //        {
                //            statistics.incNumberOfCCSlowDownEvents();
                //            x++;
                //        }
                //        passed=Util.getCurrentTime()-iterationStart;
                //        if(stopped)return;
                //    }
                //}

                DataPacket dp = sendQueue.Dequeue();
                if (dp != null)
                {
                    //Log.Write(this.ToString(), "发送数据 PacketSequenceNumber =" + dp.getPacketSequenceNumber() + "  数据长度：" + dp.getData().Length);
                    send(dp);
                    largestSentSequenceNumber = (int)dp.getPacketSequenceNumber();

                    //if (largestSentSequenceNumber != -1)
                    //{
                    //    //Log.Write(this.ToString(), "加锁成功 largestSentSequenceNumber:" + largestSentSequenceNumber);
                    //    sendQueueStart = new CountDownLatch(1);
                    //    sendQueueStart.Await();
                    //}
                    Thread.Sleep(50);
                }
            }
	    }

        /// <summary>
        /// re-submits an entry from the sender loss list
        /// 重新提交发送过程中丢失的数据列表
        /// </summary>
        /// <param name="seqNumber"></param>
	    protected void handleResubmit(long seqNumber)
        {
		    try 
            {
			    //retransmit the packet and remove it from  the list
			    DataPacket pktToRetransmit = sendBuffer[seqNumber];
			    if(pktToRetransmit!=null)
                {
				    endpoint.doSend(pktToRetransmit);
				    statistics.incNumberOfRetransmittedDataPackets();
			    }
		    }
            catch (Exception e) 
            {
			    Log.Write(this.ToString(),"WARNING",e);
		    }
	    }

        /// <summary>
        ///  for processing EXP event (see spec. p 13)
        ///  EXP事件处理  
        /// </summary>
	    public void putUnacknowledgedPacketsIntoLossList()
        {
		    lock (sendLock) 
            {
			    foreach(long l in sendBuffer.Keys)
                {
				    senderLossList.insert(l);
			    }
		    }
	    }

        /// <summary>
        /// the next sequence number for data packets.
        /// The initial sequence number is "0"
        /// </summary>
        /// <returns></returns>
	    public long getNextSequenceNumber()
        {
            currentSequenceNumber = (int)SequenceNumber.increment(currentSequenceNumber);
		    return currentSequenceNumber;
	    }

	    public long getCurrentSequenceNumber()
        {
		    return currentSequenceNumber;
	    }
      
        /// <summary>
        /// returns the largest sequence number sent so far
        /// </summary>
        /// <returns></returns>
	    public long getLargestSentSequenceNumber()
        {
		    return largestSentSequenceNumber;
	    }
      
        /// <summary>
        /// returns the last Ack. sequence number 
        /// </summary>
        /// <returns></returns>
	    public long getLastAckSequenceNumber()
        {
		    return lastAckSequenceNumber;
	    }

	    public bool haveAcknowledgementFor(long sequenceNumber)
        {
		    return SequenceNumber.compare(sequenceNumber,lastAckSequenceNumber)<=0;
	    }

	    public bool isSentOut(long sequenceNumber)
        {
		    return SequenceNumber.compare(largestSentSequenceNumber,sequenceNumber)>=0;
	    }

	    public bool haveLostPackets()
        {
		    return !senderLossList.isEmpty();
	    }

      
        /// <summary>
        /// wait until the given sequence number has been acknowledged
        /// 等到序列号被 acknowledged
        /// </summary>
        /// <param name="sequenceNumber"></param>
	    public void waitForAck(long sequenceNumber)
        {
            try
            {
                while (!session.isShutdown() && !haveAcknowledgementFor(sequenceNumber))
                {
                    waitForSeqAckLatch.Set(new CountDownLatch(1));
                    waitForSeqAckLatch.Get().Await(10);
                }
            }
            catch (Exception exc)
            {
                Log.Write(this.ToString(), exc);
            }
	    }

        /// <summary>
        /// wait for the next acknowledge
        /// 等待下一个 acknowledge
        /// </summary>
	    public void waitForAck()
        {
		    waitForAckLatch.Set(new CountDownLatch(1));
		    waitForAckLatch.Get().Await(2);
	    }

	    public void stop()
        {
		    stopped=true;
	    }
	
	    public void pause()
        {
		    startLatch=new CountDownLatch(1);
		    paused=true;
	    }

        /// <summary>
        /// 返回发送队列的数量
        /// </summary>
        /// <returns></returns>
        public int GetsendQueueCount()
        {
            int isum = 0;
            isum = sendQueue.Count;
            return isum;
        }
    }
}
