using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace udtCSharp.Common
{
    /// <summary>
    ///  holds a floating mean timing value (measured in microseconds)
    /// </summary>
    public class MeanValue 
    {
	    private double mean=0;
	
	    private int n=0;
	
	    private bool verbose;
	
	    private long nValue; 
	    private long start;
	
	    private String msg;
	
	    private String name;
	
        //public MeanValue(String name)
        //{
        //    this(name, false, 64);
        //}
	
        //public MeanValue(String name, bool verbose)
        //{
        //    this(name, verbose, 64);
        //}	

	    public MeanValue(String name, bool verbose, int nValue)
        {
            //format=NumberFormat.getNumberInstance(Locale.ENGLISH);
            //format.setMaximumFractionDigits(2);
            //format.setGroupingUsed(false);
		    this.verbose=verbose;
		    this.nValue=nValue;
		    this.name=name;
	    }
	
	    public void addValue(double value)
        {
		    mean=(mean*n+value)/(n+1);
		    n++;
		    if(verbose &&  n % nValue == 0)
            {
			    if(msg!=null)Log.Write(this.ToString(), msg+" "+getFormattedMean());
                else Log.Write(this.ToString(), name + getFormattedMean());
		    }
	    }
	
	    public double getMean()
        {
		    return mean;
	    }
	
	    public String getFormattedMean()
        {
		    return getMean().ToString();
	    }
	
	    public void clear()
        {
		    mean=0;
		    n=0;
	    }
	
	    public void begin()
        {
		    start=Util.getCurrentTime();
	    }
	
	    public void end()
        {
		    if(start>0)addValue(Util.getCurrentTime()-start);
	    }
	    public void end(String msg)
        {
		    this.msg=msg;
		    addValue(Util.getCurrentTime()-start);
	    }
	
	    public String getName()
        {
		    return name;
	    }
	
	    public String toString()
        {
		    return name;
	    }
    }
}
