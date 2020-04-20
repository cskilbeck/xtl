package ledapp.main.data.message;

import java.nio.ByteBuffer;

import ledapp.main.data.IState;

public class CMDState implements IState {

    byte b = 0;

    @Override
    public void Stash(ByteBuffer buffer) {
        buffer.put(b);
    }

    @Override
    public void Unstash(ByteBuffer buffer) {
        b = buffer.get();
    }

    @Override
    public short Size() {
        return 1;
    }
}
