package ledapp.main.data.params;

import java.nio.ByteBuffer;

import ledapp.main.data.IState;

public class Color implements IState {
    public byte r, g, b;

    @Override
    public void Stash(ByteBuffer buffer) {
        buffer.put(r);
        buffer.put(g);
        buffer.put(b);
    }

    @Override
    public void Unstash(ByteBuffer buffer) {
        r = buffer.get();
        g = buffer.get();
        b = buffer.get();
    }

    @Override
    public short Size() {
        return 3;
    }
}
