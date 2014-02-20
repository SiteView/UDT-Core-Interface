using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace udtCSharp
{
    public interface ICollect : IDisposable
    {
        bool IsDisposed { get; }
    }
}
