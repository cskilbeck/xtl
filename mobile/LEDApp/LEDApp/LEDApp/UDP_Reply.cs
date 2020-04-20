using System.Runtime.InteropServices;

namespace LEDApp
{
    // Note: this must exactly track struct udp_reply in esp project lights.ino
    [StructLayout(LayoutKind.Sequential, Pack = 1)]
    public class UDP_Reply
    {
        public uint ident;                  // fixed 0xC301DEAD
        public uint index;                  // index of last received packet
        public uint spi_receive_errors;     // how many SPI receive errors so far

        //spi_upload_packet:
        public uint spi_errors;             // SPI send errors (tracked on STM103)
        public uint frames;                 // frame count 60Hz
        public ushort adc_value;            // ADC temp reading
        public ushort pad;

        public UDP_Reply()
        {
            ident = 0;
        }
    }
}
