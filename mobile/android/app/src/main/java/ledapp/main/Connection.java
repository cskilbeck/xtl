package ledapp.main;

public interface Connection {
    void connect();

    void disconnect();

    void send(byte[] data);

    void setConnectionListener(ConnectionListener listener);
}
