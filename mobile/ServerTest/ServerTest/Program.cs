using System;
using System.Diagnostics;
using System.Net;
using System.Net.Sockets;
using System.Runtime.InteropServices;

#pragma warning disable IDE0044 // Add readonly modifier

public class ServerTest
{
    public enum message_type: byte
    {
        Nop = 0,    // dummy message, ack it, but do nothing about it
        Req = 1,    // contains some data for the channel
        Ack = 2,    // request for data from the channel
        Data = 3    // acknowledgement that version xx of data arrived on channel (size + version are what are being acknowledged)
    }

    public enum message_channel: byte
    {
        led_channel_0 = 0,
        led_channel_1 = 1,
        led_channel_2 = 2,
        led_channel_3 = 3,
        led_channel_4 = 4,
        led_channel_5 = 5,
        stm_status_channel = 6,
        max_channel = 7
    }

    [StructLayout(LayoutKind.Sequential, Pack = 1, Size = 6)]
    public class message_header
    {
        public message_type type;           // 0 message_type, see enum above
        public message_channel address;     // 1 message_channel where it's going
        public ushort version;              // 2,3 version, monotonically increasing, it wraps, that's ok
        public ushort size;                 // 4,5 size in bytes of the message (including this header)
    }

    [StructLayout(LayoutKind.Sequential, Pack = 1, Size = 3)]
    public class color
    {
        public byte r, g, b;
    }

    [StructLayout(LayoutKind.Sequential, Pack = 1, Size = 6)]
    public class color_range
    {
        public color low = new color(), high = new color();
    }

    [StructLayout(LayoutKind.Sequential, Pack = 1, Size = 4)]
    public class speed_range
    {
        public ushort low, high;
    }

    [StructLayout(LayoutKind.Sequential, Pack = 1, Size = 2 + 6 * 8 + 4 * 8)]   // 88 byte led_state packet
    public class led_state
    {
        public byte string_index;
        public byte effect_index;

        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 8)]
        public color_range[] color_ranges = new color_range[8];

        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 8)]
        public speed_range[] speed_ranges = new speed_range[8];

        public led_state()
        {
            for (int i = 0; i < 8; ++i)
            {
                color_ranges[i] = new color_range();
            }
            for (int i = 0; i < 8; ++i)
            {
                speed_ranges[i] = new speed_range();
            }
        }

        public override string ToString()
        {
            return $"{{\"string\":{string_index},\"effect\":{effect_index}}}";
        }
    }

    [StructLayout(LayoutKind.Sequential, Pack = 1, Size = 8)]
    public class stm_state
    {
        public ushort spi_errors;
        public ushort adc_value;
        public ushort frame_count;
        public ushort pad;
    }

    public class data_buffer
    {
        public data_buffer(int size)
        {
            data = new byte[size];
            offset = 0;
            length = size;
        }

        public data_buffer(byte[] packet)
        {
            data = packet;
            offset = 0;
            length = packet.Length;
        }

        public byte[] data;
        public int length;
        public int offset;

        public void take<T>(ref T dst)
        {
            int size = Marshal.SizeOf(typeof(T));
            IntPtr hglobal = Marshal.AllocHGlobal(size);
            Marshal.Copy(data, offset, hglobal, size);
            Marshal.PtrToStructure(hglobal, dst);
            Marshal.FreeHGlobal(hglobal);
            offset += size;
        }

        public void put<T>(ref T obj)
        {
            int size = Marshal.SizeOf(typeof(T));
            IntPtr hglobal = Marshal.AllocHGlobal(size);
            Marshal.StructureToPtr(obj, hglobal, false);
            Marshal.Copy(hglobal, data, offset, size);
            Marshal.FreeHGlobal(hglobal);
            offset += size;
        }
    }

    public interface state_handler
    {
        void on_data(message_header m, data_buffer din, data_buffer dout);
        void on_req(message_header m, data_buffer din, data_buffer dout);
        void on_ack(message_header m, data_buffer din, data_buffer dout);
        void on_nop(message_header m, data_buffer din, data_buffer dout);

        void handle(message_header m, data_buffer din, data_buffer dout);
    }

    public class tracked_state<T> : state_handler where T:new()
    {
        public T state;
        public int current_version = 23;
        public int acked_version;

        public message_header ack = new message_header()
        {
            type = message_type.Ack,
            size = (ushort)Marshal.SizeOf(typeof(message_header))
        };

        public void on_data(message_header m, data_buffer din, data_buffer dout)
        {
            // nab it
            din.take(ref state);
            current_version = m.version;

            // ack it
            ack.address = m.address;
            ack.version = m.version;
            dout.put(ref ack);
        }

        public void on_req(message_header m, data_buffer din, data_buffer dout)
        {
            // send back current state
            message_header msg = new message_header()
            {
                address = m.address,
                size = (ushort)(Marshal.SizeOf(typeof(message_header)) + Marshal.SizeOf(typeof(T))),
                type = message_type.Data,
                version = (ushort) current_version
            };
            dout.put(ref msg);
            dout.put(ref state);
        }

        public void on_ack(message_header m, data_buffer din, data_buffer dout)
        {
            // get the ack
            din.take(ref ack);
            acked_version = ack.version;
        }

        public void on_nop(message_header m, data_buffer din, data_buffer dout)
        {
        }

        public void handle(message_header m, data_buffer din, data_buffer dout)
        {
            // peek at the type to route it
            switch (m.type)
            {
                case message_type.Nop:
                    on_nop(m, din, dout);
                    break;
                case message_type.Req:
                    on_req(m, din, dout);
                    break;
                case message_type.Ack:
                    on_ack(m, din, dout);
                    break;
                case message_type.Data:
                    on_data(m, din, dout);
                    break;
            }
        }
    }

    static public tracked_state<led_state>[] led = new tracked_state<led_state>[6]
    {
        new tracked_state<led_state>(),
        new tracked_state<led_state>(),
        new tracked_state<led_state>(),
        new tracked_state<led_state>(),
        new tracked_state<led_state>(),
        new tracked_state<led_state>(),
    };

    static public tracked_state<stm_state> stm = new tracked_state<stm_state>();

    static public state_handler[] handlers = new state_handler[]
    {
        led[0],
        led[1],
        led[2],
        led[3],
        led[4],
        led[5],
        stm
    };

    static int handle_packet(data_buffer din, data_buffer dout)
    {
        dout.offset = 0;
        din.offset = 0;
        message_header m = new message_header();
        while (din.offset < din.length)
        {
            din.take(ref m);
            handlers[(int)m.address].handle(m, din, dout);
        }
        return dout.offset;
    }

    public static void Main()
    {
        IPEndPoint ip_endpoint = new IPEndPoint(IPAddress.Any, 9173);
        UdpClient client_socket = new UdpClient(ip_endpoint);
        IPEndPoint sender = new IPEndPoint(IPAddress.Any, 0);

        Console.WriteLine("Waiting for a client...");

        for (int i = 0; i < 6; ++i)
        {
            led[i].state = new led_state();
            led[i].state.string_index = (byte)i;
            led[i].state.color_ranges[0].low = new color() { r = 1, g = 2, b = 3 };
            led[i].state.color_ranges[0].high = new color() { r = 250, g = 251, b = 252 };
            led[i].state.speed_ranges[0].low = 1234;
            led[i].state.speed_ranges[0].high = 5678;
        }

        stm.state = new stm_state();
        stm.state.adc_value = 123;
        stm.state.frame_count = 456;

        data_buffer dout = new data_buffer(4096);

        while (true)
        {
            byte[] recv_data = client_socket.Receive(ref sender);
            data_buffer din = new data_buffer(recv_data);
            Console.WriteLine($"Got {recv_data.Length} bytes from {sender.Address}:{sender.Port}");
            int length = handle_packet(din, dout);
            client_socket.Send(dout.data, dout.offset, sender);
        }
    }
}
