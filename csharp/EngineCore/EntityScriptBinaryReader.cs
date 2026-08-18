using System;
using System.Buffers.Binary;
using System.Text;

namespace vkEngine.EngineCore
{
    /// <summary>Low-level reader used by generated EntityScript deserializers.</summary>
    public ref struct EntityScriptBinaryReader
    {
        public const int MaxStringByteLength = 256 * 1024 * 1024;
        public const int MaxArrayElementCount = 16_777_216;
        public const int MaxByteArrayElementCount = 1_073_741_824;

        private static readonly UTF8Encoding strictUtf8 = new(
            encoderShouldEmitUTF8Identifier: false,
            throwOnInvalidBytes: true);

        private readonly ReadOnlySpan<byte> data;
        private int offset;

        public unsafe EntityScriptBinaryReader(byte* data, int dataSize)
        {
            if (dataSize < 0)
                throw new ArgumentOutOfRangeException(nameof(dataSize));
            if (data == null && dataSize != 0)
                throw new ArgumentNullException(nameof(data));

            this.data = new ReadOnlySpan<byte>(data, dataSize);
            offset = 0;
        }

        public int Offset => offset;
        public int Length => data.Length;

        public void Align(int alignment)
        {
            if (alignment <= 0 || (alignment & (alignment - 1)) != 0)
                throw new ArgumentOutOfRangeException(nameof(alignment));

            int aligned;
            try
            {
                aligned = checked((offset + alignment - 1) & -alignment);
            }
            catch (OverflowException exception)
            {
                throw InvalidData("alignment overflows the data offset", exception);
            }

            Require(aligned - offset);
            for (; offset < aligned; offset++)
            {
                if (data[offset] != 0)
                    throw InvalidData("alignment padding contains a non-zero byte");
            }
        }

        public byte ReadByte()
        {
            Require(1);
            return data[offset++];
        }

        public int ReadInt32()
        {
            Align(4);
            ReadOnlySpan<byte> bytes = Take(4);
            return BinaryPrimitives.ReadInt32LittleEndian(bytes);
        }

        public long ReadInt64()
        {
            Align(8);
            ReadOnlySpan<byte> bytes = Take(8);
            return BinaryPrimitives.ReadInt64LittleEndian(bytes);
        }

        public float ReadFloat32()
        {
            return BitConverter.Int32BitsToSingle(ReadInt32());
        }

        public double ReadFloat64()
        {
            return BitConverter.Int64BitsToDouble(ReadInt64());
        }

        public string ReadString()
        {
            int byteLength = ReadLength(MaxStringByteLength, "string byte length");
            ReadOnlySpan<byte> bytes = Take(byteLength);
            try
            {
                return strictUtf8.GetString(bytes);
            }
            catch (DecoderFallbackException exception)
            {
                throw InvalidData("string is not valid UTF-8", exception);
            }
        }

        public int ReadArrayLength(bool byteArray)
        {
            return ReadLength(
                byteArray ? MaxByteArrayElementCount : MaxArrayElementCount,
                "array element count");
        }

        public void ValidateArrayPayload(int count, int minimumElementSize)
        {
            if (count < 0)
                throw new ArgumentOutOfRangeException(nameof(count));
            if (minimumElementSize < 0)
                throw new ArgumentOutOfRangeException(nameof(minimumElementSize));

            int minimumByteLength;
            try
            {
                minimumByteLength = checked(count * minimumElementSize);
            }
            catch (OverflowException exception)
            {
                throw InvalidData("array payload size overflows Int32", exception);
            }
            Require(minimumByteLength);
        }

        public void EnsureComplete()
        {
            if (offset != data.Length)
                throw InvalidData($"root value consumed {offset} of {data.Length} bytes");
        }

        private int ReadLength(int maximum, string description)
        {
            int value = ReadInt32();
            if (value < 0)
                throw InvalidData($"{description} is negative");
            if (value > maximum)
                throw InvalidData($"{description} exceeds the format limit of {maximum}");
            return value;
        }

        private ReadOnlySpan<byte> Take(int byteCount)
        {
            Require(byteCount);
            ReadOnlySpan<byte> result = data.Slice(offset, byteCount);
            offset += byteCount;
            return result;
        }

        private void Require(int byteCount)
        {
            if (byteCount < 0 || byteCount > data.Length - offset)
                throw InvalidData($"read of {byteCount} byte(s) at offset {offset} exceeds the {data.Length}-byte buffer");
        }

        private InvalidOperationException InvalidData(string message, Exception? inner = null)
        {
            return new InvalidOperationException(
                $"Invalid EntityScript binary data at offset {offset}: {message}.", inner);
        }
    }
}
