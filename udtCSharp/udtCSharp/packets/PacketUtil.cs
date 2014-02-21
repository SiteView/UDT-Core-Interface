using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace udtCSharp.packets
{
    public class PacketUtil
    {
        public static byte[] encode(long value)
        {
            byte m4 = (byte)(value >> 24);
            byte m3 = (byte)(value >> 16);
            byte m2 = (byte)(value >> 8);
            byte m1 = (byte)(value);
            return new byte[] { m4, m3, m2, m1 };
        }

        public static byte[] encodeSetHighest(bool highest, long value)
        {
            byte m4;
            if (highest)
            {
                m4 = (byte)(0x80 | value >> 24);
            }
            else
            {
                m4 = (byte)(0x7f & value >> 24);
            }
            byte m3 = (byte)(value >> 16);
            byte m2 = (byte)(value >> 8);
            byte m1 = (byte)(value);
            return new byte[] { m4, m3, m2, m1 };
        }


        public static byte[] encodeControlPacketType(int type)
        {
            byte m4 = (byte)0x80;

            byte m3 = (byte)type;
            return new byte[] { m4, m3, 0, 0 };
        }


        public static long decode(byte[] data, int start)
        {
            long result = (data[start] & 0xFF) << 24
                         | (data[start + 1] & 0xFF) << 16
                         | (data[start + 2] & 0xFF) << 8
                         | (data[start + 3] & 0xFF);
            return result;
        }


        public static int decodeType(byte[] data, int start)
        {
            int result = data[start + 1] & 0xFF;
            return result;
        }
    }
}
