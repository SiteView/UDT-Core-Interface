using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace udtCSharp.Common
{
    /// <summary>
    /// 拥有一个浮点数的平均值
    /// </summary>
    public class MeanThroughput : MeanValue
    {
	
	    private double packetSize;
	
	    public MeanThroughput(String name, int packetSize):this(name, false, 64, packetSize)
        {
		    
	    }
	
	    public MeanThroughput(String name, bool verbose, int packetSize):this(name, verbose, 64, packetSize)
        {
		    ;
	    }
	
	    public MeanThroughput(String name, bool verbose, int nValue, int packetSize):base(name,verbose,nValue)
        {
		    this.packetSize=packetSize;
	    }


	    public override double getMean() 
        {
		    return packetSize/base.getMean();
	    }
	

    }
}
