package ledapp.main.data.message;

import android.util.Log;

import java.nio.ByteBuffer;

import ledapp.main.data.IState;

public class StateHandler {

    final static String TAG = "StateHandler";

    public IState obj;
    public int currentVersion;
    public int ackedVersion;
    public int address;

    public StateHandler(IState o, int address) {
        obj = o;
        this.address = address;
    }

    public void Handle(MessageHeader m, ByteBuffer din, ByteBuffer dout) {
        Log.i(TAG, String.format("MSG Addr: %d", m.address));

        MessageHeader r = new MessageHeader();

        switch (m.type) {
            case MessageType.Ack:
                Log.i(TAG, String.format("    ACK Version: %d", m.version));
                ackedVersion = m.version;
                break;

            case MessageType.Req:
                Log.i(TAG, "    REQ");
                r.type = MessageType.Data;
                r.version = (short) currentVersion;
                r.size = (short) (obj.Size() + 4);
                r.address = m.address;
                r.Stash(dout);
                obj.Stash(dout);
                break;

            case MessageType.Data:
                Log.i(TAG, String.format("    DATA Size: %d, Version: %d", m.size, m.version));
                currentVersion = m.version;
                obj.Unstash(din);
                r.type = MessageType.Ack;
                r.address = m.address;
                r.size = 4;
                r.version = (short) currentVersion;
                r.Stash(dout);
                break;

            case MessageType.Cmd:
                Log.i(TAG, "    NOP");
                break;
        }
    }
}
