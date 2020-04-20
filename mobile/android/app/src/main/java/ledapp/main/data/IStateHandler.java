package ledapp.main.data;

import java.nio.ByteBuffer;

import ledapp.main.data.message.MessageHeader;

public interface IStateHandler {
    void on_data(ByteBuffer in, ByteBuffer out);

    void on_req(ByteBuffer in, ByteBuffer out);

    void on_ack(ByteBuffer in, ByteBuffer out);

    void on_nop(ByteBuffer in, ByteBuffer out);

    void handle(MessageHeader msg, ByteBuffer in, ByteBuffer out);
}
