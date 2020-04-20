package ledapp.main;

import java.nio.ByteBuffer;

public interface ConnectionListener {
    void onConnected();

    void onDisconnected();

    void onConnectionFailed(String errorMsg);

    void onDataReceived(ByteBuffer data);
}
