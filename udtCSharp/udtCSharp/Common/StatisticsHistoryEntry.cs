using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace udtCSharp.Common
{
    public class StatisticsHistoryEntry 
    {

	    private object[] values;

	    private long timestamp;

	    private bool isHeading;
	
	    public StatisticsHistoryEntry(bool heading, long time, object[] values)
        {
		    this.values=values;
		    this.isHeading=heading;
		    this.timestamp=time;
	    }

	    public StatisticsHistoryEntry(bool heading, long time, List<MeanValue>metrics)
        {
		    this.isHeading=heading;
		    this.timestamp=time;
		    int length=metrics.Count;
		    if(isHeading)length++;
		    object[]metricValues=new object[length];
		    if(isHeading){
			    metricValues[0]="time";
			    for(int i=0;i<metrics.Count;i++){
				    metricValues[i+1]=metrics[i].getName();
			    }
		    }
		    else{
			    for(int i=0;i<metricValues.Length;i++){
				    metricValues[i]=metrics.[i].getFormattedMean();
			    }
		    }
		    this.values=metricValues;
	    }

        //public StatisticsHistoryEntry(object values)
        //{
        //    this(false,System.currentTimeMillis(),values);
        //}

	    /**
	     * output as comma separated list
	     */
	    public String toString()
        {
		    StringBuilder sb=new StringBuilder();
		    if(!isHeading)
            {
			    sb.Append(timestamp);
			    foreach(object val in values)
                {
                    sb.Append(" , ").Append(val.ToString());
			    }
		    }
		    else
            {
			    for(int i=0;i<values.Length;i++)
                {
				    if(i>0)sb.Append(" , ");
                    sb.Append(values[i].ToString());
			    }
		    }
		    return sb.ToString();
	    }

    }
}
