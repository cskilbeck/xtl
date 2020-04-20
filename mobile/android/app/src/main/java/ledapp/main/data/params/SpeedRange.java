package ledapp.main.data.params;

import java.nio.ByteBuffer;

import ledapp.main.data.IState;

public class SpeedRange implements IState {
    public short from, to;

    @Override
    public void Stash(ByteBuffer buffer) {
        buffer.putShort(from);
        buffer.putShort(to);
    }

    @Override
    public void Unstash(ByteBuffer buffer) {
        from = buffer.getShort();
        to = buffer.getShort();
    }

    @Override
    public short Size() {
        return 4;
    }
}
