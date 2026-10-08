using System;
using System.Buffers;
using System.Buffers.Binary;
using System.Text;

namespace vkEngine.EngineCore
{
    /// <summary>Writes the same aligned little-endian format as EntityScriptBinaryReader.</summary>
    public sealed class EntityScriptBinaryWriter
    {
        private readonly ArrayBufferWriter<byte> buffer = new();
        private static readonly UTF8Encoding utf8 = new(false, true);

        public void Align(int alignment)
        {
            if (alignment <= 0 || (alignment & (alignment - 1)) != 0)
                throw new ArgumentOutOfRangeException(nameof(alignment));
            int count = checked((buffer.WrittenCount + alignment - 1) & -alignment) - buffer.WrittenCount;
            if (count == 0) return;
            buffer.GetSpan(count).Slice(0, count).Clear();
            buffer.Advance(count);
        }

        public void WriteByte(byte value)
        {
            buffer.GetSpan(1)[0] = value;
            buffer.Advance(1);
        }

        public void WriteByteArray(byte[] value)
        {
            ArgumentNullException.ThrowIfNull(value);
            WriteArrayLength(value.Length, byteArray: true);
            if (value.Length == 0) return;
            value.AsSpan().CopyTo(buffer.GetSpan(value.Length));
            buffer.Advance(value.Length);
        }

        public void WriteInt32(int value)
        {
            Align(4);
            BinaryPrimitives.WriteInt32LittleEndian(buffer.GetSpan(4), value);
            buffer.Advance(4);
        }

        public void WriteInt64(long value)
        {
            Align(8);
            BinaryPrimitives.WriteInt64LittleEndian(buffer.GetSpan(8), value);
            buffer.Advance(8);
        }

        public void WriteFloat32(float value)
        {
            if (!float.IsFinite(value)) throw new InvalidOperationException("Cannot export a non-finite float.");
            WriteInt32(BitConverter.SingleToInt32Bits(value));
        }

        public void WriteFloat64(double value)
        {
            if (!double.IsFinite(value)) throw new InvalidOperationException("Cannot export a non-finite double.");
            WriteInt64(BitConverter.DoubleToInt64Bits(value));
        }

        public void WriteString(string value)
        {
            ArgumentNullException.ThrowIfNull(value);
            int count = utf8.GetByteCount(value);
            if (count > EntityScriptBinaryReader.MaxStringByteLength)
                throw new InvalidOperationException("String exceeds the binary format limit.");
            WriteInt32(count);
            if (count == 0) return;
            utf8.GetBytes(value.AsSpan(), buffer.GetSpan(count));
            buffer.Advance(count);
        }

        public void WriteArrayLength(int count, bool byteArray)
        {
            int limit = byteArray ? EntityScriptBinaryReader.MaxByteArrayElementCount : EntityScriptBinaryReader.MaxArrayElementCount;
            if (count < 0 || count > limit)
                throw new InvalidOperationException("Array exceeds the binary format limit.");
            WriteInt32(count);
        }

        public byte[] ToArray() => buffer.WrittenSpan.ToArray();
    }
}
