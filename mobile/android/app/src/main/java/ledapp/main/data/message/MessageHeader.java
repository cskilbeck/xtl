package ledapp.main.data.message;

import java.nio.ByteBuffer;

import ledapp.main.data.IState;

public class MessageHeader implements IState {
    @Override
    public void Stash(ByteBuffer buffer) {
        buffer.put(type);
        buffer.put(address);
        buffer.putShort(version);
        buffer.putShort(size);
    }

    @Override
    public void Unstash(ByteBuffer buffer) {
        type = buffer.get();
        address = buffer.get();
        version = buffer.getShort();
        size = buffer.getShort();
    }

    @Override
    public short Size() {
        // TODO (chs): use annotations and reflection for this
        return 6;
    }

    public byte type;           // 0 message_type, see above
    public byte address;        // 1 message_channel where it's going, see above
    public short version;       // 2,3 version, monotonically increasing, it wraps, that's ok
    public short size;          // 4,5 size in bytes of the message (including this header)
}
