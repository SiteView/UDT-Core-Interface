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

        public MeanValue(String name)
            : this(name, false, 64)
        {
            ;
        }

        public MeanValue(String name, bool verbose)
            : this(name, verbose, 64)
        {
            ;
        }	

	    public MeanValue(String name, bool verbose, int nValue)
        {
            //format=NumberFormat.getNumberInstance(Locale.ENGLISH);//返回指定语言环境的通用数值格式。
            //format.setMaximumFractionDigits(2);//保留两位小数
            //format.setGroupingUsed(false);//设置此格式中是否使用分组。
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
			    if(msg!=null)
                    Log.Write(this.ToString(), msg+" "+getFormattedMean());
                else
                    Log.Write(this.ToString(), name + getFormattedMean());
		    }
	    }
	
	    public virtual double getMean()
        {
		    return mean;
	    }
	
        /// <summary>
        /// 返回一个保留两位小数的字符串
        /// </summary>
        /// <returns></returns>
	    public String getFormattedMean()
        {
		    return Math.Round(getMean(),2).ToString();
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
