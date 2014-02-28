using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.IO;

namespace udtCSharp.packets
{
     /**
     * NAK carries information about lost packets
     * 
     * loss info is described in the spec on p.15
     */
    public class NegativeAcknowledgement : ControlPacket
    {

	    //after decoding this contains the lost sequence numbers
	    List<int>lostSequenceNumbers;

	    //this contains the loss information intervals as described on p.15 of the spec
	    //byte[] lossInfo;
        List<byte> lossInfo = new List<byte>();//最后在将数据转换为 byte[] lossInfo;

	    public NegativeAcknowledgement()
        {
		    this.controlPacketType=(int)ControlPacketType.NAK;
	    }

	    public NegativeAcknowledgement(byte[]controlInformation):base()
        {
		    lostSequenceNumbers=decode(controlInformation);
	    }
	
	    /**
	     * decode the loss info
	     * @param lossInfo
	     */
	    private List<int> decode(byte[]lossInfo)
        {
		    List<int> lostSequenceNumbers = new List<int>();

            int lossInfo_length = lossInfo.Length;
            int readinfo_length = 0;
		    byte[]buffer=new byte[4];
            while (lossInfo_length > 0)
            {
			    //read 4 bytes
                buffer[0] = lossInfo[readinfo_length];
                readinfo_length++;
                lossInfo_length--;
                buffer[1] = lossInfo[readinfo_length];
                readinfo_length++;
                lossInfo_length--;
                buffer[2] = lossInfo[readinfo_length];
                readinfo_length++;
                lossInfo_length--;
                buffer[3] = lossInfo[readinfo_length];
                readinfo_length++;
                lossInfo_length--;

			    bool isNotSingle=(buffer[0]&128)!=0;
			    //set highest bit back to 0
			    buffer[0]=(byte)(buffer[0]&0x7f);
                int lost = BitConverter.ToInt32(buffer,0);

			    if(isNotSingle)
                {
				    //get the end of the interval
                    int end = BitConverter.ToInt32(buffer, readinfo_length - 4);
				    //and add all lost numbers to the result list
				    for(int i=lost;i<=end;i++)
                    {
					    lostSequenceNumbers.Add(i);
				    }
			    }
                else
                {
				    lostSequenceNumbers.Add(lost);
			    }
		    }
		    return lostSequenceNumbers;
	    }

	    /**
	     * add a single lost packet number
	     * @param singleSequenceNumber
	     */
	    public void addLossInfo(long singleSequenceNumber) 
        {
		    byte[] enc=PacketUtil.encodeSetHighest(false, singleSequenceNumber);
		    try
            {
                for (int i = 0; i < enc.Length;i++ )
                    lossInfo.Add(enc[i]);
		    }
            catch(Exception ignore)
            {
                Log.Write(this.ToString(),ignore);
            }
	    }

	    /**
	     * add an interval of lost packet numbers
	     * @param firstSequenceNumber
	     * @param lastSequenceNumber
	     */
	    public void addLossInfo(long firstSequenceNumber, long lastSequenceNumber) 
        {
		    //check if we really need an interval
		    if(lastSequenceNumber-firstSequenceNumber==0){
			    addLossInfo(firstSequenceNumber);
			    return;
		    }
		    //else add an interval
		    byte[] enc1=PacketUtil.encodeSetHighest(true, firstSequenceNumber);
		    byte[] enc2=PacketUtil.encodeSetHighest(false, lastSequenceNumber);
		    try
            {
                for (int i = 0; i < enc1.Length; i++)
			    lossInfo.Add(enc1[i]);
                for (int i = 0; i < enc2.Length; i++)
			    lossInfo.Add(enc2[i]);
		    }
            catch(Exception ignore)
            {
                Log.Write(this.ToString(), ignore);
            }
	    }

	    /**
	     * pack the given list of sequence numbers and add them to the loss info
	     * @param sequenceNumbers - a list of sequence numbers
	     */
	    public void addLossInfo(List<long> sequenceNumbers)
        {
		    long start=0;
		    int index=0;
		    do
            {
			    start=sequenceNumbers[index];
			    long end=0;
			    int c=0;
			    do
                {
				    c++;
				    index++;
				    if(index<sequenceNumbers.Count)
                    {
					    end=sequenceNumbers[index];
				    }
			    }
                while(end-start==c);
			    if(end==0)
                {
				    addLossInfo(start);
			    }
			    else
                {
				    end=sequenceNumbers[index-1];
				    addLossInfo(start, end);
			    }
		    }
            while(index<sequenceNumbers.Count);
	    }

	    /**
	     * Return the lost packet numbers
	     * @return
	     */
	    public List<int> getDecodedLossInfo() 
        {
		    return lostSequenceNumbers;
	    }

	    
	    public override byte[] encodeControlInformation()
        {
		    try 
            {
                if (lossInfo.Count > 0)
                {
                    byte[] loss = new byte[lossInfo.Count];
                    for (int i = 0; i < lossInfo.Count; i++)
                    {
                        loss[i] = lossInfo[i];
                    }
                    return loss;
                }
                else
                {
                    return null; ;
                }
		    } 
            catch (Exception e) 
            {
			    // can't happen
                Log.Write(this.ToString(), "can't happen", e);
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
		    NegativeAcknowledgement other = (NegativeAcknowledgement) obj;
		
		    List<int>thisLost=null;
		    List<int>otherLost=null;
		
		    //compare the loss info
		    if(lostSequenceNumbers!=null)
            {
			    thisLost=lostSequenceNumbers;
		    }
            else
            {
                if (lossInfo.Count > 0)
                {
                    byte[] loss = new byte[lossInfo.Count];
                    for (int i = 0; i < lossInfo.Count; i++)
                    {
                        loss[i] = lossInfo[i];
                    }
                    thisLost = decode(loss);
                }
		    }
		    if(other.lostSequenceNumbers!=null)
            {
			    otherLost=other.lostSequenceNumbers;
		    }
            else
            {
                if (other.lossInfo.Count > 0)
                {
                    byte[] loss = new byte[other.lossInfo.Count];
                    for (int i = 0; i < other.lossInfo.Count; i++)
                    {
                        loss[i] = other.lossInfo[i];
                    }
                    otherLost = other.decode(loss);
                }			    
		    }
		    if(!thisLost.Equals(otherLost))
            {
			    return false;
		    }

		    return true;
	    }


    }
}
