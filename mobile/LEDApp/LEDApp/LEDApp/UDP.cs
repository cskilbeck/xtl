using System;
using System.Net.Sockets;
using System.Threading;
using System.Net;
using System.Runtime.InteropServices;

namespace LEDApp
{
    public class UDP
    {
        // up yet?
        bool connected = false;

        // most recent version set
        uint current_version;

        // most recent version that server has acknowledged
        uint acked_version;

        // most recent version we sent
        uint sent_version;

        // connection to the server
        UdpClient socket = new UdpClient();

        // server endpoint
        string server_host;
        int server_port;
        IPAddress server_ip;

        Parameters current_parameters = new Parameters();

        // current latest version of the state
        byte[] current_state;

        // for synchronizing with state setters
        object state_lock = new object();

        MainViewModel model_;

        public UDP(MainViewModel model)
        {
            model_ = model;
        }

        // for waking up when state changes
        AutoResetEvent wait_event = new AutoResetEvent(false);

        public static byte[] get_buffer<T>(ref T obj)
        {
            int size = Marshal.SizeOf(typeof(T));
            IntPtr hglobal = Marshal.AllocHGlobal(size);
            Marshal.StructureToPtr(obj, hglobal, false);
            byte[] buffer = new byte[size];
            Marshal.Copy(hglobal, buffer, 0, size);
            Marshal.FreeHGlobal(hglobal);
            return buffer;
        }

        public static T from_buffer<T>(byte[] src) where T: new()
        {
            int size = Marshal.SizeOf(typeof(T));
            //Log.Debug($"from_buffer, len = {size}");
            if (src.Length != size)
            {
                return default(T);
            }
            else
            {
                IntPtr hglobal = Marshal.AllocHGlobal(size);
                Marshal.Copy(src, 0, hglobal, size);
                T dst = new T();
                Marshal.PtrToStructure(hglobal, dst);
                Marshal.FreeHGlobal(hglobal);
                return dst;
            }
        }

        public void start_sending(string host, int port)
        {
            server_host = host;
            server_port = port;
            server_ip = IPAddress.Parse(host);
            Log.Info($"Starting send thread on {host}:{port}");
            new Thread(client_loop).Start();
        }

        // call this to ask it to send a new version of the state
        public void set_state(Parameters new_state)
        {
            //Log.Debug($"set_state, index before = {current_version}");
            lock (state_lock)
            {
                current_parameters = new_state;
                current_version += 1;
                current_parameters.header.index = current_version;
                new_state.header.index = current_version;
                current_state = get_buffer(ref current_parameters);
                wait_event.Set();
            }
        }

        // just re-send to get the reply
        public void update()
        {
            current_parameters.header.id = 0x01C3;
            current_parameters.header.index += 1;
            current_parameters.header.string_id = 0;
            current_parameters.header.effect = 0;
            set_state(current_parameters);
        }

        public void receive_thread()
        {
            IPEndPoint from = new IPEndPoint(IPAddress.Any, server_port);
            UDP_Reply reply = new UDP_Reply();
            while (true)
            {
                try
                {
                    byte[] got = socket.Receive(ref from);
                    if (from.Address.Equals(server_ip) && 
                        from.Port == server_port && 
                        got.Length == Marshal.SizeOf(typeof(UDP_Reply)))
                    {
                        reply = from_buffer<UDP_Reply>(got);
                        if (reply.ident == 0xC301DEAD)
                        {
                            acked_version = Math.Max(acked_version, reply.index);
                            //Log.Verbose($"ACK: {acked_version}");
                            model_.DebugString = $"F:{reply.frames} T:{reply.adc_value} E:{reply.spi_errors}";
                        }
                    }
                    else
                    {
                        Log.Info($"Spurious from {from.Address}:{from.Port}, {got.Length} bytes");
                    }
                }
                catch (SocketException e)
                {
                    if(e.SocketErrorCode != SocketError.TimedOut)
                    {
                        Log.Error($"SocketException {e.SocketErrorCode}");
                    }
                }
            }
        }

        // thread
        void client_loop()
        {
            Log.Info($"Connect to {server_host}:{server_port}");

            int connect_tries = 0;
            while (connect_tries < 20)
            {
                try
                {
                    socket.Connect(server_host, server_port);
                    connected = true;
                    break;
                }
                catch (SocketException e)
                {
                    Log.Error($"Can't CONNECT: {e.Message}");
                    Thread.Sleep(1000);
                    connect_tries += 1;
                }
            }

            if (connected)
            {
                int delay = 1000;

                acked_version = 0;
                current_version = 0;

                socket.DontFragment = true;
                socket.Client.SendTimeout = 200;
                socket.Client.ReceiveTimeout = 5000;

                new Thread(receive_thread).Start();

                int retries = 0;

                while (true)
                {
                    lock (state_lock)
                    {
                        // if we think this version might not be there yet
                        if (current_version != acked_version)
                        {
                            // then send it (maybe again (and again, ad infinitum))
                            delay = 250;
                            //Log.Debug($"Sending version {current_version} (for the {retries} th time)");
                            if (current_version != sent_version)
                            {
                                retries = 0;
                                sent_version = current_version;
                            }
                            retries += 1;
                            try
                            {
                                socket.Send(current_state, current_state.Length);
                            }
                            catch (SocketException e)
                            {
                                if (e.SocketErrorCode == SocketError.TimedOut)
                                {
                                    // ok, timeout is one thing
                                    Log.Warn($"SOCKET SAYS: {e.Message}");
                                }
                                else
                                {
                                    // something more serious...
                                    Log.Error($"FATAL SOCKET ERROR: {e.Message}");
                                }
                            }
                        }
                        else
                        {
                            //Log.Verbose("Nothing new to send...");
                        }
                    }

                    // loop delay
                    wait_event.WaitOne(delay);
                    delay = 5000;
                }
            }
        }
    }
}
