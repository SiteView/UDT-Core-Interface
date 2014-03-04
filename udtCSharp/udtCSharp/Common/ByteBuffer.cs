using System;
using System.Collections.Generic;
using System.Collections;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace udtCSharp.Common
{
    /// <summary>
    /// 创建一个可变长的Byte数组方便Push数据和Pop数据
    /// 数组的最大长度为1024,超过会产生溢出
    /// 数组的最大长度由常量MAX_LENGTH设定
    /// 
    /// 注:由于实际需要,可能要从左到右取数据,所以这里
    /// 定义的Pop函数并不是先进后出的函数,而是从0开始.
    /// </summary>
    public class ByteBuffer
    {
        /// <summary>
        /// 数组的最大长度(默认1024字节)
        /// </summary>
        private int MAX_LENGTH = 1024;

        /// <summary>
        /// 固定长度的中间数组
        /// </summary>
        private byte[] TEMP_BYTE_ARRAY;

        /// <summary>
        /// 当前数组长度
        /// </summary>
        private int CURRENT_LENGTH = 0;

        /// <summary>
        /// 当前Pop指针位置
        /// </summary>
        private int CURRENT_POSITION = 0;

        /// <summary>
        /// 最后返回数组
        /// </summary>
        private byte[] RETURN_ARRAY;

        /// <summary>
        /// 默认构造函数生成一个1024字节的Byte数组
        /// </summary>
        public ByteBuffer()
        {
            this.Initialize();
        }       

        /// <summary>
        /// 重载的构造函数,用一个Byte数组来构造
        /// </summary>
        /// <param name="bytes">用于构造ByteBuffer的数组</param>
        public ByteBuffer(byte[] bytes)
        {
            this.MAX_LENGTH = bytes.Length;
            this.Initialize();
            this.PushByteArray(bytes);
        }
      
        /// <summary>
        /// 重载的构造函数,用一个Byte数组来构造
        /// </summary>
        /// <param name="bytes">用于构造ByteBuffer的数组</param>
        /// <param name="offset">要使用的子数组的偏移量</param>
        /// <param name="length">要使用的子数组的长度</param>
        public ByteBuffer(byte[] bytes,int offset, int length)
        {
            this.MAX_LENGTH = bytes.Length;
            this.Initialize();
            this.CURRENT_POSITION = offset;           
            this.PushByteArray(bytes);
        }

        /// <summary>
        /// 重载的构造函数,用一个指定的长度构造Byte数组
        /// </summary>
        /// <param name="capacity">用于构造ByteBuffer的数组长度</param>
        public ByteBuffer(int capacity)
        {
            this.MAX_LENGTH = capacity;
            this.Initialize();
        }

        /// <summary>
        /// 初始化ByteBuffer的每一个元素,并把当前指针指向头一位
        /// </summary>
        public void Initialize()
        {
            TEMP_BYTE_ARRAY = new byte[MAX_LENGTH];
            TEMP_BYTE_ARRAY.Initialize();
            CURRENT_LENGTH = 0;
            CURRENT_POSITION = 0;
        }

        /// <summary>
        /// 获取当前ByteBuffer的长度
        /// </summary>
        public int Length
        {
            get
            {
                return CURRENT_LENGTH;
            }
        }

        /// <summary>
        /// 获取/设置当前出栈指针位置
        /// </summary>
        public int Position
        {
            get
            {
                return CURRENT_POSITION;
            }
            set
            {
                CURRENT_POSITION = value;
            }
        }
        /// <summary>
        /// 返回剩余的可用长度，此长度为实际能读取的数据长度
        /// </summary>
        /// <returns></returns>
        public int Remaining() 
        {
            return this.MAX_LENGTH - this.CURRENT_POSITION;
        }

        /// <summary>
        /// 将缓冲区的字节传输到给定的目标数组中
        /// </summary>
        /// <param name="bytes">给定的目标数组</param>
        public void Get(ref byte[] bytes)
        {
            Array.Copy(TEMP_BYTE_ARRAY, this.CURRENT_POSITION, bytes, 0, bytes.Length);
            this.CURRENT_POSITION += bytes.Length;
        }

        /// <summary>
        /// 获取ByteBuffer所生成的数组
        /// 长度必须小于 [MAXSIZE]
        /// </summary>
        /// <returns>Byte[]</returns>
        public byte[] ToByteArray()
        {
            //分配大小
            RETURN_ARRAY = new byte[CURRENT_LENGTH];
            //调整指针
            Array.Copy(TEMP_BYTE_ARRAY, 0, RETURN_ARRAY, 0, CURRENT_LENGTH);
            return RETURN_ARRAY;
        }

      

        /// <summary>
        /// 向ByteBuffer压入一个字节
        /// </summary>
        /// <param name="by">一位字节</param>
        public void PushByte(byte by)
        {
            TEMP_BYTE_ARRAY[CURRENT_LENGTH++] = by;
        }

        /// <summary>
        /// 向ByteBuffer压入数组
        /// </summary>
        /// <param name="ByteArray">数组</param>
        public void PushByteArray(byte[] ByteArray)
        {
            //把自己CopyTo目标数组
            ByteArray.CopyTo(TEMP_BYTE_ARRAY, CURRENT_LENGTH);
            //调整长度
            this.CURRENT_LENGTH += ByteArray.Length;
        }

        /// <summary>
        /// 向ByteBuffer压入两字节的Short
        /// </summary>
        /// <param name="Num">2字节Short</param>
        public void PushUInt16(UInt16 Num)
        {
            TEMP_BYTE_ARRAY[CURRENT_LENGTH++] = (byte)(((Num & 0xff00) >> 8) & 0xff);
            TEMP_BYTE_ARRAY[CURRENT_LENGTH++] = (byte)((Num & 0x00ff) & 0xff);
        }

        /// <summary>
        /// 向ByteBuffer压入一个无符Int值
        /// </summary>
        /// <param name="Num">4字节UInt32</param>
        public void PushInt(UInt32 Num)
        {
            TEMP_BYTE_ARRAY[CURRENT_LENGTH++] = (byte)(((Num & 0xff000000) >> 24) & 0xff);
            TEMP_BYTE_ARRAY[CURRENT_LENGTH++] = (byte)(((Num & 0x00ff0000) >> 16) & 0xff);
            TEMP_BYTE_ARRAY[CURRENT_LENGTH++] = (byte)(((Num & 0x0000ff00) >> 8) & 0xff);
            TEMP_BYTE_ARRAY[CURRENT_LENGTH++] = (byte)((Num & 0x000000ff) & 0xff);
        }

        /// <summary>
        /// 向ByteBuffer压入一个Long值
        /// </summary>
        /// <param name="Num">4字节Long</param>
        public void PushLong(long Num)
        {
            TEMP_BYTE_ARRAY[CURRENT_LENGTH++] = (byte)(((Num & 0xff000000) >> 24) & 0xff);
            TEMP_BYTE_ARRAY[CURRENT_LENGTH++] = (byte)(((Num & 0x00ff0000) >> 16) & 0xff);
            TEMP_BYTE_ARRAY[CURRENT_LENGTH++] = (byte)(((Num & 0x0000ff00) >> 8) & 0xff);
            TEMP_BYTE_ARRAY[CURRENT_LENGTH++] = (byte)((Num & 0x000000ff) & 0xff);
        }

        /// <summary>
        /// 从ByteBuffer的当前位置弹出一个Byte,并提升一位
        /// </summary>
        /// <returns>1字节Byte</returns>
        public byte PopByte()
        {
            byte ret = TEMP_BYTE_ARRAY[CURRENT_POSITION++];
            return ret;
        }

        /// <summary>
        /// 从ByteBuffer的当前位置弹出一个Short,并提升两位
        /// </summary>
        /// <returns>2字节Short</returns>
        public UInt16 PopUInt16()
        {
            //溢出
            if (CURRENT_POSITION + 1 >= CURRENT_LENGTH)
            {
                return 0;
            }
            UInt16 ret = (UInt16)(TEMP_BYTE_ARRAY[CURRENT_POSITION] << 8 | TEMP_BYTE_ARRAY[CURRENT_POSITION + 1]);
            CURRENT_POSITION += 2;
            return ret;
        }

        /// <summary>
        /// 从ByteBuffer的当前位置弹出一个uint,并提升4位
        /// </summary>
        /// <returns>4字节UInt</returns>
        public uint PopUInt()
        {
            if (CURRENT_POSITION + 3 >= CURRENT_LENGTH)
                return 0;
            uint ret = (uint)(TEMP_BYTE_ARRAY[CURRENT_POSITION] << 24 | TEMP_BYTE_ARRAY[CURRENT_POSITION + 1] << 16 | TEMP_BYTE_ARRAY[CURRENT_POSITION + 2] << 8 | TEMP_BYTE_ARRAY[CURRENT_POSITION + 3]);
            CURRENT_POSITION += 4;
            return ret;
        }

        /// <summary>
        /// 从ByteBuffer的当前位置弹出一个long,并提升4位
        /// </summary>
        /// <returns>4字节Long</returns>
        public long PopLong()
        {
            if (CURRENT_POSITION + 3 >= CURRENT_LENGTH)
                return 0;
            long ret = (long)(TEMP_BYTE_ARRAY[CURRENT_POSITION] << 24 | TEMP_BYTE_ARRAY[CURRENT_POSITION + 1] << 16 | TEMP_BYTE_ARRAY[CURRENT_POSITION + 2] << 8 | TEMP_BYTE_ARRAY[CURRENT_POSITION + 3]);
            CURRENT_POSITION += 4;
            return ret;
        }

        /// <summary>
        /// 从ByteBuffer的当前位置弹出长度为Length的Byte数组,提升Length位
        /// </summary>
        /// <param name="Length">数组长度</param>
        /// <returns>Length长度的byte数组</returns>
        public byte[] PopByteArray(int Length)
        {
            //溢出
            if (CURRENT_POSITION + Length >= CURRENT_LENGTH)
            {
                return new byte[0];
            }
            byte[] ret = new byte[Length];
            Array.Copy(TEMP_BYTE_ARRAY, CURRENT_POSITION, ret, 0, Length);
            //提升位置
            CURRENT_POSITION += Length;
            return ret;
        }

    }
}
