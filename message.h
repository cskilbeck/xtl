//////////////////////////////////////////////////////////////////////
// MOBILE APPLICATION <--> ESP12 <--> STM103

// UDP layer    - MOBILE <--> ESP12         packets transferred as required
// SPI layer    - ESP12 <--> STM103         1K transferred in each direction every 60Hz

// transfer buffer: either a UDP packet or a SPI packet
// integrity has been checked by the time we get to this bit
// transfer buffer consists of some messages, each with a message_header

//////////////////////////////////////////////////////////////////////
// a message is targeted at a channel, has an action and a version (for state tracking)

#pragma pack(push, 1)
struct message_header
{
    uint8 length;           // in uint32s, up to 1024 bytes
    uint8 address;          // who this message is for
    uint16 version : 14;    // incremented if state is new
    uint16 action : 2;      // see enum

    size_t message_length() const
    {
        return length * 4;
    }
};
#pragma pack(pop)

//////////////////////////////////////////////////////////////////////
// a message has an action

enum action
{
    action_data = 0,       // here is the current state of the data in the channel
    action_ack = 1,        // message received
    action_req = 2,        // request for current state (version is ignored)
    action_reserved = 3    // for future use
};

//////////////////////////////////////////////////////////////////////
// state which can be replicated around the place
// one per string of leds
// one for stm103 status
// etc

//////////////////////////////////////////////////////////////////////
// message_handler for outgoing messages

struct message_handler
{
    uint16 current_version;
    uint16 sent_version;        // also, kind of, aka received_version
    uint16 acked_version;
    uint8 address;
    uint8 pad;

    // platform specific handlers
    virtual void on_data(message_header *msg) = 0;
    virtual void on_ack(message_header *msg) = 0;
    virtual void on_req(message_header *msg) = 0;

    void process_message(message_header *msg)
    {
        switch(msg->action) {
        case action_data:
            handler->on_data(msg);
            break;
        case action_ack:
            handler->on_ack(msg);
            break;
        case action_req:
            handler->on_req(msg);
            break;
        }
    }
};

//////////////////////////////////////////////////////////////////////
// led handler

struct led_message_handler : message_handler
{
    void on_data(message_header *msg);
    void on_ack(message_header *msg);
    void on_req(message_header *msg);
};

//////////////////////////////////////////////////////////////////////
// status handler

struct status_message_handler : message_handler
{
    void on_data(message_header *msg);
    void on_ack(message_header *msg);
    void on_req(message_header *msg);
};

//////////////////////////////////////////////////////////////////////
// for message routing

struct channel_handler
{
    uint32 channel;
    message_handler *handler;
};

//////////////////////////////////////////////////////////////////////
// the message channel IDs

enum
{
    channel_none = 0,
    channel_led_0 = 1,
    channel_led_1 = 2,
    channel_led_2 = 3,
    channel_led_3 = 4,
    channel_led_4 = 5,
    channel_led_5 = 6,
    channel_status = 7
};

//////////////////////////////////////////////////////////////////////
// these are platform specific

extern led_message_handler led_handler;
extern status_message_handler status_handler;

//////////////////////////////////////////////////////////////////////
// on the ESP12, we need to deal with messages incoming from the MOBILE APP via UDP
// and from the STM103 via SPI...

// message channel handler table

// same handler for each led string, it can identify the string
// from the message_header.address

// clang-format off
channel_handler handlers[] = {
    { channel_led_0, led_handler },
    { channel_led_1, led_handler },
    { channel_led_2, led_handler },
    { channel_led_3, led_handler },
    { channel_led_4, led_handler },
    { channel_led_5, led_handler },
    { channel_status, status_handler },
    { channel_none, null  }
};
// clang-format on

//////////////////////////////////////////////////////////////////////
// get the handler for a channel

channel_handler *find_handler(channel_handler *handlers, int channel)
{
    while(handlers->channel != channel_none) {
        if(handlers->channel == channel) {
            return handlers->handler;
        }
        handlers += 1;
    }
    return null;
}

//////////////////////////////////////////////////////////////////////
// handle a buffer with 0 or more messages in it

void process_messages(byte *buffer, int buffer_length, channel_handler *handlers)
{
    byte *end = buffer + buffer_length;
    while(buffer < end) {
        message_header *msg = (message_header *)buffer;
        channel_handler *handler = find_handler(handlers, msg->channel);
        if(handler != null) {
            handler->process_message(msg);
        }
        buffer += msg->message_length();
    }
}

//////////////////////////////////////////////////////////////////////

byte *add_data(int length, byte *buffer, int &buffer_length, int max_buffer_length)
{
    int remain = max_buffer_length - buffer_length;
    if(remain < length) {
        return null;
    }
    byte *p = buffer + buffer_length;
    buffer_length += length;
    return p;
}

//////////////////////////////////////////////////////////////////////

bool send_data(int address, int channel, byte *data, int data_length, int &buffer_length, int max_buffer_length)
{

}

//////////////////////////////////////////////////////////////////////

bool send_status_request(int address, int channel, byte *buffer, int &buffer_length, int max_buffer_length)
{
    message_header *req_message = (message_header *)add_data(sizeof(message_header), buffer, buffer_length, max_buffer_length);
    if(req_message != null) {
        req_message->length = sizeof(message_header);
        req_message->address = address;
        req_message->version = 0;           // requests have no version, they should always be actioned
        req_message->action = action_req;
        return true;
    }
    return false;
}

//////////////////////////////////////////////////////////////////////
// All the handlers:

// STM103

// status_handler   -> ESP12        send status to ESP12
// led_handler      <- ESP12        get led parameters from ESP12

// ESP12

// status_handler   <- STM103       get status from STM103
// led_handler      -> STM103       send led parameters to STM103
// led_handler      <- MOBILE       get led parameters from MOBILE

// MOBILE

// status_handler   <- ESP12        get status from ESP12
// led_handler      -> ESP12        send led parameters to ESP12
