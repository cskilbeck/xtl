using System;
using System.Runtime.InteropServices;

namespace LEDApp
{
    [StructLayout(LayoutKind.Sequential, Pack = 1)]
    public struct color
    {
        public byte r, g, b;
    }

    [StructLayout(LayoutKind.Sequential, Pack = 1)]
    public struct color_range
    {
        public color low;
        public color high;
    }

    [StructLayout(LayoutKind.Sequential, Pack = 1)]
    public struct speed_range
    {
        public ushort low;
        public ushort high;
    }

    [StructLayout(LayoutKind.Sequential, Pack = 1)]
    public struct message_header
    {
        public uint index;          // UDP state index (if this changes, we got a new one from the phone app)
        public ushort id;           // 0x01C3
        public byte string_id;      // on this string
        public byte effect;         // set this effect
    }

    [StructLayout(LayoutKind.Sequential, Pack = 1), Serializable]
    public struct Parameters
    {
        const int max_color_parameters = 8;
        const int max_speed_parameters = 8;

        [MarshalAs(UnmanagedType.Struct)]
        public message_header header;

        [MarshalAs(UnmanagedType.ByValArray, ArraySubType = UnmanagedType.Struct, SizeConst = 8)]
        public color_range[] color_ranges;

        [MarshalAs(UnmanagedType.ByValArray, ArraySubType = UnmanagedType.Struct, SizeConst = 8)]
        public speed_range[] speed_ranges;

        public void reset(int string_id)
        {
            header.string_id = (byte)string_id;
            color_ranges = new color_range[max_color_parameters];
            speed_ranges = new speed_range[max_speed_parameters];
            header.id = 0x01C3;
        }

        public void set_string_id(int id)
        {
            header.string_id = (byte)id;
            Log.Debug($"Setting ID to {header.string_id}");
        }

        public void set_speed_range(int p, ushort low, ushort high)
        {
            speed_ranges[p].low = low;
            speed_ranges[p].high = high;
        }

        public void set_color_range(int p, byte rlow, byte glow, byte blow, byte rhigh, byte ghigh, byte bhigh)
        {
            color_ranges[p].low.r = rlow;
            color_ranges[p].low.g = glow;
            color_ranges[p].low.b = blow;
            color_ranges[p].high.r = rhigh;
            color_ranges[p].high.g = ghigh;
            color_ranges[p].high.b = bhigh;
        }

        public void set_effect(int effect)
        {
            header.effect = (byte)effect;
        }
    }
}
