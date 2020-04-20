#pragma once

#include "types.h"

namespace message
{

    enum message_type
    {
        message_Cmd = 0,    // command message, it wants you to do something
        message_Req = 1,    // contains some data for the channel
        message_Ack = 2,    // request for data from the channel
        message_Data = 3    // acknowledgement that version xx of data arrived on channel (size + version are what are being acknowledged)
    };

    enum message_channel
    {
        led_channel_0 = 0,
        led_channel_1 = 1,
        led_channel_2 = 2,
        led_channel_3 = 3,
        led_channel_4 = 4,
        led_channel_5 = 5,
        stm_status_channel = 6,
        global_channel = 7,
        max_channel = 8
    };

#pragma pack(push, 1)
#pragma anon_unions
    struct message_header
    {
        byte type;         // 0 message_type, see enum above
        byte address;      // 1 message_channel where it's going
        uint16 version;    // 2,3 version, monotonically increasing, it wraps, that's ok
        uint16 size;       // 4,5 size in bytes of the message (including this header)
    };

    struct color
    {
        union
        {
            struct
            {
                byte r, g, b;
            };
            struct
            {
                byte h, s, v;
            };
        };

        color() : r(0), g(0), b(0)
        {
        }

        color(byte red, byte green, byte blue) : r(red), g(green), b(blue)
        {
        }

        void set(uint32 c)
        {
            r = (c >> 16) & 0xff;
            g = (c >> 8) & 0xff;
            b = c & 0xff;
        }

        color(uint32 c)
        {
            set(c);
        }

        color rgb_from_hsv();
        color hsv_from_rgb();
    };

    struct color_range
    {
        color low;
        color high;
    };

    struct speed_range
    {
        uint16 low;
        uint16 high;
    };

    enum global_enum
    {
        global_power = 1,       // 1 = on, 0 = off
        global_pause = 2,       // 1 = paused, 0, not paused
        global_new_state = 4,   // 1 = there's new state to be had, reset all led effects, 0 = state is the same
        global_debug_led = 8    // set state of debug (orange) led
    };

    struct global_state
    {
        uint8 flags;    // see global_enum
        uint8 brightness;
        uint8 speed;
        uint8 pad;
    };

    struct led_state
    {
        byte string_index;
        byte effect_index;

        color_range color_ranges[8];
        speed_range speed_ranges[8];
    };

    // 13 = solid

    struct stm_state
    {
        uint16 spi_errors;
        uint16 adc_value;
        uint16 frame_count;
        uint16 pad;
    };

    struct data_buffer
    {
        enum Mode
        {
            mode_read = 0,
            mode_write = 1
        };

        byte *data;
        uint16 position;
        uint16 length;
        uint16 capacity;

        Mode mode;

        data_buffer(byte *buffer, uint16 capacity)
        {
            data = buffer;
            position = 0;
            length = 0;
            this->capacity = capacity;
            mode = mode_write;
        }

        void clear()
        {
            mode = mode_write;
            position = 0;
            length = 0;
        }

        void read()
        {
            mode = mode_read;
            position = 0;
        }

        template <typename T> void take(T *dst)
        {
            // assert(mode == mode_read);
            memcpy((byte *)dst, data + position, sizeof(T));
            position += sizeof(T);
        }

        void put(byte *src_data, int amount)
        {
            // assert(mode == mode_write);
            memcpy(data + position, src_data, amount);
            position += amount;
        }

        template <typename T> void put(T *obj)
        {
            put((byte *)obj, sizeof(T));
        }
    };

    struct state_handler
    {
        virtual void on_data(message_header m, data_buffer &din, data_buffer &dout) = 0;
        virtual void on_req(message_header m, data_buffer &din, data_buffer &dout) = 0;
        virtual void on_ack(message_header m, data_buffer &din, data_buffer &dout) = 0;
        virtual void on_cmd(message_header m, data_buffer &din, data_buffer &dout) = 0;

        virtual void handle(message_header m, data_buffer &din, data_buffer &dout) = 0;
        virtual void request(int address, data_buffer &dout) = 0;
        virtual void dump(int address, data_buffer &dout) = 0;

        void send_ack(message_header m, data_buffer &dout)
        {
            message_header ack;
            ack.type = message_Ack;
            ack.address = m.address;
            ack.version = m.version;
            ack.size = (uint16)sizeof(message_header);
            dout.put(&ack);
        }
    };

    template <typename T> struct tracked_state : state_handler
    {
        T state;
        int current_version;
        int acked_version;

        virtual void on_data_arrived(data_buffer &din, data_buffer &dout)
        {
        }

        virtual void on_data(message_header m, data_buffer &din, data_buffer &dout)
        {
            // nab it
            din.take(&state);
            current_version = m.version;
            send_ack(m, dout);
            on_data_arrived(din, dout);
        }

        virtual void on_req(message_header m, data_buffer &din, data_buffer &dout)
        {
            dump(m.address, dout);
        }

        virtual void on_ack(message_header m, data_buffer &din, data_buffer &dout)
        {
            // get the ack
            message_header ack;
            din.take(&ack);
            acked_version = ack.version;
        }

        virtual void on_cmd(message_header m, data_buffer &din, data_buffer &dout)
        {
        }

        virtual void dump(int address, data_buffer &dout)
        {
            message_header msg;
            msg.address = (byte)address;
            msg.size = (uint16)(sizeof(message_header) + sizeof(T));
            msg.type = message_Data;
            msg.version = (uint16)current_version;
            dout.put(&msg);
            dout.put(&state);
        }

        virtual void request(int address, data_buffer &dout)
        {
            message_header msg;
            msg.address = (byte)address;
            msg.size = (uint16)(sizeof(message_header));
            msg.type = message_Req;
            msg.version = 0;
            dout.put(&msg);
        }

        virtual void handle(message_header m, data_buffer &din, data_buffer &dout)
        {
            // peek at the type to route it
            switch(m.type) {
            case message_Cmd:
                on_cmd(m, din, dout);
                break;
            case message_Req:
                on_req(m, din, dout);
                break;
            case message_Ack:
                on_ack(m, din, dout);
                break;
            case message_Data:
                on_data(m, din, dout);
                break;
            }
        }
    };
#pragma pack(pop)

    inline color color::rgb_from_hsv()
    {
        if(s == 0) {
            return color(v, v, v);
        }

        int region = h / 43;
        int remainder = (h - (region * 43)) * 6;

        int p = (v * (255 - s)) >> 8;
        int q = (v * (255 - ((s * remainder) >> 8))) >> 8;
        int t = (v * (255 - ((s * (255 - remainder)) >> 8))) >> 8;

        color rgb;

        switch(region) {
        case 0:
            rgb.r = v;
            rgb.g = t;
            rgb.b = p;
            break;
        case 1:
            rgb.r = q;
            rgb.g = v;
            rgb.b = p;
            break;
        case 2:
            rgb.r = p;
            rgb.g = v;
            rgb.b = t;
            break;
        case 3:
            rgb.r = p;
            rgb.g = q;
            rgb.b = v;
            break;
        case 4:
            rgb.r = t;
            rgb.g = p;
            rgb.b = v;
            break;
        default:
            rgb.r = v;
            rgb.g = p;
            rgb.b = q;
            break;
        }

        return rgb;
    }

    inline color color::hsv_from_rgb()
    {
        color hsv;

        byte mn = r < g ? (r < b ? r : b) : (g < b ? g : b);
        byte mx = r > g ? (r > b ? r : b) : (g > b ? g : b);

        hsv.v = mx;
        if(hsv.v == 0) {
            hsv.h = 0;
            hsv.s = 0;
            return hsv;
        }

        hsv.s = 255 * int32(mx - mn) / hsv.v;
        if(hsv.s == 0) {
            hsv.h = 0;
            return hsv;
        }

        int mm = mx - mn;
        if(mx == r) {
            hsv.h = 0 + 43 * (g - b) / mm;
        } else if(mx == g) {
            hsv.h = 85 + 43 * (b - r) / mm;
        } else {
            hsv.h = 171 + 43 * (r - g) / mm;
        }

        return hsv;
    }
}    // namespace message

