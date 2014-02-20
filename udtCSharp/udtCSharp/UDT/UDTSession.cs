using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace udtCSharp.UDT
{
    public class UDTSession
    {
        protected int mode;
	    protected volatile bool active;
	    private volatile int state=start;
	    protected volatile UDTPacket lastPacket;
	
	    //state constants	
	    public static  int start=0;
	    public static  int handshaking=1;
	    public static  int ready=2;
	    public static  int keepalive=3;
	    public static  int shutdown=4;
	
	    public static  int invalid=99;

	    protected volatile UDTSocket socket;
	
	    protected  UDTStatistics statistics;
	
	    protected int receiveBufferSize=64*32768;
	
	    protected CongestionControl cc;
	
	    //cache dgPacket (peer stays the same always)
	    private DatagramPacket dgPacket;

	    /**
	     * flow window size, i.e. how many data packets are
	     * in-flight at a single time
	     */
	    protected int flowWindowSize=1024;

	    /**
	     * remote UDT entity (address and socket ID)
	     */
	    protected Destination destination;
	
	    /**
	     * local port
	     */
	    protected int localPort;
	
	
	    public static int DEFAULT_DATAGRAM_SIZE=UDPEndPoint.DATAGRAM_SIZE;
	
	    /**
	     * key for a system property defining the CC class to be used
	     * @see CongestionControl
	     */
	    public static String CC_CLASS="udt.congestioncontrol.class";
	
	    /**
	     * Buffer size (i.e. datagram size)
	     * This is negotiated during connection setup
	     */
	    protected int datagramSize=DEFAULT_DATAGRAM_SIZE;
	
	    protected long initialSequenceNumber=null;
	
	    protected long mySocketID;
	
	    private static AtomicLong nextSocketID=new AtomicLong(20+new Random().nextInt(5000));
	
        //public UDTSession(String description, Destination destination){
        //    statistics=new UDTStatistics(description);
        //    mySocketID=nextSocketID.incrementAndGet();
        //    this.destination=destination;
        //    this.dgPacket=new DatagramPacket(new byte[0],0,destination.getAddress(),destination.getPort());
        //    String clazzP=System.getProperty(CC_CLASS,UDTCongestionControl.class.getName());
        //    Object ccObject=null;
        //    try{
        //        Class<?>clazz=Class.forName(clazzP);
        //        ccObject=clazz.getDeclaredConstructor(UDTSession.class).newInstance(this);
        //    }catch(Exception e){
        //        logger.log(Level.WARNING,"Can't setup congestion control class <"+clazzP+">, using default.",e);
        //        ccObject=new UDTCongestionControl(this);
        //    }
        //    cc=(CongestionControl)ccObject;
        //    logger.info("Using "+cc.getClass().getName());
        //}
	
	
        //public abstract void received(UDTPacket packet, Destination peer);
	
	
        //public UDTSocket getSocket() {
        //    return socket;
        //}

        //public CongestionControl getCongestionControl() {
        //    return cc;
        //}

        //public int getState() {
        //    return state;
        //}

        //public void setMode(int mode) {
        //    this.mode = mode;
        //}

        //public void setSocket(UDTSocket socket) {
        //    this.socket = socket;
        //}

        //public void setState(int state) {
        //    logger.info(toString()+" connection state CHANGED to <"+state+">");
        //    this.state = state;
        //}
	
        //public boolean isReady(){
        //    return state==ready;
        //}

        //public boolean isActive() {
        //    return active == true;
        //}

        //public void setActive(boolean active) {
        //    this.active = active;
        //}
	
        //public boolean isShutdown(){
        //    return state==shutdown || state==invalid;
        //}
	
        //public Destination getDestination() {
        //    return destination;
        //}
	
        //public int getDatagramSize() {
        //    return datagramSize;
        //}

        //public void setDatagramSize(int datagramSize) {
        //    this.datagramSize = datagramSize;
        //}
	
        //public int getReceiveBufferSize() {
        //    return receiveBufferSize;
        //}

        //public void setReceiveBufferSize(int bufferSize) {
        //    this.receiveBufferSize = bufferSize;
        //}

        //public int getFlowWindowSize() {
        //    return flowWindowSize;
        //}

        //public void setFlowWindowSize(int flowWindowSize) {
        //    this.flowWindowSize = flowWindowSize;
        //}

        //public UDTStatistics getStatistics(){
        //    return statistics;
        //}

        //public long getSocketID(){
        //    return mySocketID;
        //}

	
        //public synchronized long getInitialSequenceNumber(){
        //    if(initialSequenceNumber==null){
        //        initialSequenceNumber=1l; //TODO must be random?
        //    }
        //    return initialSequenceNumber;
        //}
	
        //public synchronized void setInitialSequenceNumber(long initialSequenceNumber){
        //    this.initialSequenceNumber=initialSequenceNumber;
        //}

        //public DatagramPacket getDatagram(){
        //    return dgPacket;
        //}
	
        //public String toString(){
        //    StringBuilder sb=new StringBuilder();
        //    sb.append(super.toString());
        //    sb.append(" [");
        //    sb.append("socketID=").append(this.mySocketID);
        //    sb.append(" ]");
        //    return sb.toString();
        //}
    }
}
