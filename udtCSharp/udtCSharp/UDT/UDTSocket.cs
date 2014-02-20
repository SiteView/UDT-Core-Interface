using System;
using System.Collections;
using System.Collections.Generic;
using System.ComponentModel;
using System.Globalization;
using System.Linq;
using System.Net;
using System.Net.Sockets;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Security.Permissions;
using System.Text;
using System.Xml.Linq; 

namespace udtCSharp.UDT
{
    public sealed class UDTSocket : Collect
    {

        protected override void SingleDispose()
        {
            throw new NotImplementedException();
        }

        //endpoint
	    private UDPEndPoint endpoint;
	
	    private boolean active;
	
        ////processing received data
        //private UDTReceiver receiver;
        //private UDTSender sender;
	
        //private final UDTSession session;

        //private UDTInputStream inputStream;
        //private UDTOutputStream outputStream;



    }
}
