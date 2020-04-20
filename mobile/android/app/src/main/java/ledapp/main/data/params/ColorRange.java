package ledapp.main.data.params;

import java.nio.ByteBuffer;

import ledapp.main.data.IState;

public class ColorRange implements IState {
    public Color from = new Color();
    public Color to = new Color();

    @Override
    public void Stash(ByteBuffer buffer) {
        from.Unstash(buffer);
        to.Unstash(buffer);
    }

    @Override
    public void Unstash(ByteBuffer buffer) {
        from.Stash(buffer);
        to.Stash(buffer);
    }

    @Override
    public short Size() {
        return (short) (from.Size() + to.Size());
    }
}
