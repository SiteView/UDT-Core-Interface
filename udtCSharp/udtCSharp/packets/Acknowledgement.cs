using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace udtCSharp.packets
{
     /**
     * Acknowledgement is sent by the {@link UDTReceiver} to the {@link UDTSender} to acknowledge
     * receipt of packets
     */
    public class Acknowledgement : ControlPacket 
    {
	    //the ack sequence number
	    private long ackSequenceNumber ;

	    //the packet sequence number to which all the previous packets have been received (excluding)
	    private long ackNumber ;

	    //round-trip time in microseconds(RTT)
	    private long roundTripTime;
	    // RTT variance
	    private long roundTripTimeVariance;
	    //Available buffer size (in bytes)
	    private long bufferSize;
	    //packet receivind rate in number of packets per second
	    private long pktArrivalSpeed;
	    //estimated link capacity in number of packets per second
	    private long estimatedLinkCapacity;

	    public Acknowledgement()
        {
		    this.controlPacketType= (int)ControlPacketType.ACK;
	    }

	    public Acknowledgement(long ackSeqNo, byte[] controlInformation):base()
        {
		    this.ackSequenceNumber=ackSeqNo;
		    decodeControlInformation(controlInformation);
	    }

	    void decodeControlInformation(byte[] data)
        {
		    ackNumber=PacketUtil.decode(data, 0);
		    if(data.Length>4){
			    roundTripTime =PacketUtil.decode(data, 4);
			    roundTripTimeVariance = PacketUtil.decode(data, 8);
			    bufferSize = PacketUtil.decode(data, 12);
		    }
		    if(data.Length>16){
			    pktArrivalSpeed = PacketUtil.decode(data, 16);
			    estimatedLinkCapacity = PacketUtil.decode(data, 20);
		    }
	    }
	   
	    protected override long getAdditionalInfo()
        {
		    return ackSequenceNumber;
	    }

	    public long getAckSequenceNumber() 
        {
		    return ackSequenceNumber;
	    }
	    public void setAckSequenceNumber(long ackSequenceNumber) 
        {
		    this.ackSequenceNumber = ackSequenceNumber;
	    }


	    /**
	     * get the ack number (the number up to which all packets have been received (excluding))
	     * @return
	     */
	    public long getAckNumber() {
		    return ackNumber;
	    }

	    /**
	     * set the ack number (the number up to which all packets have been received (excluding))
	     * @param ackNumber
	     */
	    public void setAckNumber(long ackNumber) {
		    this.ackNumber = ackNumber;
	    }

	    /**
	     * get the round trip time (microseconds)
	     * @return
	     */
	    public long getRoundTripTime() {
		    return roundTripTime;
	    }
	    /**
	     * set the round trip time (in microseconds)
	     * @param RoundTripTime
	     */
	    public void setRoundTripTime(long RoundTripTime) {
		    roundTripTime = RoundTripTime;
	    }

	    /**
	     * set the variance of the round trip time (in microseconds)
	     * @param RoundTripTime
	     */
	    public void setRoundTripTimeVar(long roundTripTimeVar) {
		    roundTripTimeVariance = roundTripTimeVar;
	    }

	    public long getRoundTripTimeVar() {
		    return roundTripTimeVariance;
	    }

	    public long getBufferSize() {
		    return bufferSize;
	    }

	    public void setBufferSize(long bufferSiZe) {
		    this.bufferSize = bufferSiZe;
	    }

	    public long getPacketReceiveRate() {
		    return pktArrivalSpeed;
	    }
	    public void setPacketReceiveRate(long packetReceiveRate) {
		    this.pktArrivalSpeed = packetReceiveRate;
	    }


	    public long getEstimatedLinkCapacity() {
		    return estimatedLinkCapacity;
	    }

	    public void setEstimatedLinkCapacity(long estimatedLinkCapacity)
        {
		    this.estimatedLinkCapacity = estimatedLinkCapacity;
	    }

	    public override byte[] encodeControlInformation()
        {
		    try 
            {
                byte[] backNumber = PacketUtil.encode(ackNumber);
                byte[] broundTripTime = PacketUtil.encode(roundTripTime);
                byte[] broundTripTimeVariance = PacketUtil.encode(roundTripTimeVariance);
                byte[] bbufferSize = PacketUtil.encode(bufferSize);
                byte[] bpktArrivalSpeed = PacketUtil.encode(pktArrivalSpeed);
                byte[] bestimatedLinkCapacity = PacketUtil.encode(estimatedLinkCapacity);
                byte[] bvalue = new byte[backNumber.Length + broundTripTime.Length + broundTripTimeVariance.Length + bbufferSize.Length + bpktArrivalSpeed.Length + bestimatedLinkCapacity.Length];
                Array.Copy(backNumber, 0, bvalue, 0, backNumber.Length);
                Array.Copy(broundTripTime, 0, bvalue, backNumber.Length, broundTripTime.Length);
                Array.Copy(broundTripTimeVariance, 0, bvalue, backNumber.Length + broundTripTime.Length, broundTripTimeVariance.Length);
                Array.Copy(bbufferSize, 0, bvalue, backNumber.Length + broundTripTime.Length + broundTripTimeVariance.Length, bbufferSize.Length);
                Array.Copy(bpktArrivalSpeed, 0, bvalue, backNumber.Length + broundTripTime.Length + broundTripTimeVariance.Length + bbufferSize.Length, bpktArrivalSpeed.Length);
                Array.Copy(bestimatedLinkCapacity, 0, bvalue, backNumber.Length + broundTripTime.Length + broundTripTimeVariance.Length + bbufferSize.Length + bpktArrivalSpeed.Length, bestimatedLinkCapacity.Length);
               
                return bvalue;
		    } 
            catch (Exception e)
            {
			    // can't happen
			    return null;
		    }
	    }
	    
	    public override bool equals(Object obj) 
        {
		    if (this == obj)
			    return true;
		    if (!base.equals(obj))
			    return false;
		    if (this.GetType() != obj.GetType())
			    return false;
		    Acknowledgement other = (Acknowledgement) obj;
		    if (ackNumber != other.ackNumber)
			    return false;
		    if (roundTripTime != other.roundTripTime)
			    return false;
		    if (roundTripTimeVariance != other.roundTripTimeVariance)
			    return false;
		    if (bufferSize != other.bufferSize)
			    return false;
		    if (estimatedLinkCapacity != other.estimatedLinkCapacity)
			    return false;
		    if (pktArrivalSpeed != other.pktArrivalSpeed)
			    return false;
		    return true;
	    }
    }
}
