#pragma once

//////////////////////////////////////////////////////////////////////

inline uint16 crc16(byte *buffer, uint32 len)
{
    uint32 crc = 0xffff;
    for(uint i=0; i<len; ++i) {
        uint32 x = ((crc>>8) ^ buffer[i]) & 0xff;
        x ^= x>>4;
        crc = (crc << 8) ^ (x << 12) ^ (x <<5) ^ x;
    }
    return (uint16)crc;
}

//////////////////////////////////////////////////////////////////////
// data_packet - for SPI transfer buffer

#pragma pack(push, 1)
struct spi_data_buffer
{
    enum
    {
        packet_size = 1020,
        transfer_size = 1024
    };

    uint16 length;
    uint16 crc;
    byte data[packet_size]; // sequential set of packets (see below)
   
    spi_data_buffer()
    {
        length = 0;
    }
    
    byte *transfer_buffer()
    {
        return (byte *)this;
    }
    
    byte *packet_buffer()
    {
        return data;
    }

    void init(int len)
    {
        length = (uint16)len;
        crc = crc16(data, len);
    }
    
    bool is_valid()
    {
        if(length > packet_size) {
            return false;
        }
        return crc16(data, length) == crc;
    }
};
#pragma pack(pop)

//////////////////////////////////////////////////////////////////////



