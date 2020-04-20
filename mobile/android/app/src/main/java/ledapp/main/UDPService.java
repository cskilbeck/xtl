package ledapp.main;

import android.os.Handler;
import android.os.Looper;
import android.util.Log;

import java.net.InetSocketAddress;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.channels.DatagramChannel;

@SuppressWarnings("WeakerAccess")
public class UDPService implements Connection {

    final static String TAG = "UDPService";

    final static int MAX_SEND_PACKET_SIZE = 4096;
    final static int MAX_UDP_PACKET_SIZE = 4096;

    final static String DEFAULT_IP = "192.168.0.15";
    final static int DEFAULT_PORT = 9173;

    DatagramChannel mChannel = null;
    ConnectionListener mConnectionListener = null;
    String mAddress = DEFAULT_IP;
    int mPort = DEFAULT_PORT;
    boolean mIsConnected = false;
    ByteBuffer mSendBuf = ByteBuffer.allocateDirect(MAX_SEND_PACKET_SIZE);
    ByteBuffer mReceiveBuf = ByteBuffer.allocateDirect(MAX_UDP_PACKET_SIZE);
    ListenerThread mListenerThread = null;
    ConnectionThread mConnectionThread = null;

    UDPService() {
        Log.i(TAG, "UDPService ctor");
        mSendBuf.order(ByteOrder.LITTLE_ENDIAN);
        mReceiveBuf.order(ByteOrder.LITTLE_ENDIAN);
    }

    public void setConnectionListener(ConnectionListener listener) {
        mConnectionListener = listener;
    }

    public void connect(String addr, int port) {
        mAddress = addr;
        mPort = port;
        if (addr.isEmpty()) {
            mAddress = DEFAULT_IP;
        }
        if (mPort <= 0 || mPort > 0xFFFF) {
            mPort = DEFAULT_PORT;
        }
        connect();
    }

    public void connect() {
        if (mConnectionThread != null) {
            return;
        }
        mConnectionThread = new ConnectionThread();
        mConnectionThread.start();
    }

    public void disconnect() {
        if (!mIsConnected) {
            return;
        }
        try {
            mListenerThread = null;
            mConnectionThread = null;
            mChannel.disconnect();
        } catch (Exception ignored) {
        }
        mIsConnected = false;
        mConnectionListener.onDisconnected();
    }

    public void send(ByteBuffer data) {
        ConnectionThread thread;
        synchronized (this) {
            if (!mIsConnected || mConnectionThread == null) {
                return;
            }
            thread = mConnectionThread;
        }
        // Perform the write unsynchronized
        thread.send(data);
    }

    public void send(byte[] data) {
        ConnectionThread thread;
        synchronized (this) {
            if (!mIsConnected || mConnectionThread == null) {
                return;
            }
            thread = mConnectionThread;
        }
        // Perform the write unsynchronized
        thread.send(data);
    }

    private class ConnectionThread extends Thread {

        private Handler mSendHandler;

        public void run() {
            if (mIsConnected) {
                return;
            }
            try {
                mChannel = DatagramChannel.open();
                mChannel.configureBlocking(true);
                mChannel.connect(new InetSocketAddress(mAddress, mPort));

                if (mListenerThread == null) {
                    mListenerThread = new ListenerThread();
                    mListenerThread.start();
                    mConnectionListener.onConnected();
                    mIsConnected = true;
                }

                Looper.prepare();
                mSendHandler = new Handler();
                Looper.loop();

            } catch (Exception e) {
                mConnectionListener.onConnectionFailed(e.toString());
            }
        }

        public void send(final ByteBuffer data) {
            mSendHandler.post(new Runnable() {
                @Override
                public void run() {
                    try {
                        data.flip();
                        mChannel.write(data);
                        data.clear();
                    } catch (Exception e) {
                        Log.i(TAG, "Send exception: " + e.getMessage());
                        disconnect();
                    }
                }
            });
        }

        public void send(final byte[] data) {
            mSendBuf.put(data);
            send(mSendBuf);
        }
    }

    private class ListenerThread extends Thread {

        public void run() {
            mReceiveBuf.order(ByteOrder.LITTLE_ENDIAN);
            while (true) {
                try {
                    mReceiveBuf.clear();
                    mChannel.read(mReceiveBuf);
                    mReceiveBuf.flip();
                    Log.i(TAG, "Received " + mReceiveBuf.limit());
                    if (mConnectionListener != null && mReceiveBuf.limit() > 0) {
                        mConnectionListener.onDataReceived(mReceiveBuf);
                    }
                    mReceiveBuf.clear();
                } catch (Exception e) {
                    disconnect();
                    break;
                }
            }
        }
    }
}
