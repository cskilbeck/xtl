package ledapp.main.data;

import java.nio.ByteBuffer;

public interface IState {
    void Stash(ByteBuffer buffer);

    void Unstash(ByteBuffer buffer);

    short Size();
}
