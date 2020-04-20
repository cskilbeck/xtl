/*
  How to implement trackedstate when generics have no 'where' clause like in C# ? Some interface gymnastics... or a shitty switch/case?
 */

package ledapp.main;

import android.content.Context;
import android.content.Intent;
import android.net.nsd.NsdManager;
import android.net.nsd.NsdServiceInfo;
import android.os.Bundle;
import android.provider.Settings;
import android.support.v7.app.AppCompatActivity;
import android.util.Log;
import android.view.View;
import android.widget.Button;

import java.net.InetAddress;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;

import ledapp.main.data.IState;
import ledapp.main.data.message.CMDState;
import ledapp.main.data.message.LEDState;
import ledapp.main.data.message.MessageChannel;
import ledapp.main.data.message.MessageHeader;
import ledapp.main.data.message.MessageType;
import ledapp.main.data.message.STMState;
import ledapp.main.data.message.StateHandler;

//import android.content.Intent;
//import android.provider.Settings;

public class MainActivity extends AppCompatActivity {

    final static String TAG = "MainActivity";

    static UDPService udpService = new UDPService();

    final static Object sync = new Object();

    public static Boolean connected = false;

    NsdManager.ResolveListener mResolveListener;
    NsdManager.DiscoveryListener mDiscoveryListener;
    String SERVICE_TYPE = "_http._tcp";
    String mServiceName = "foo";

    public static LEDState ledState0 = new LEDState();
    public static LEDState ledState1 = new LEDState();
    public static LEDState ledState2 = new LEDState();
    public static LEDState ledState3 = new LEDState();
    public static LEDState ledState4 = new LEDState();
    public static LEDState ledState5 = new LEDState();
    public static STMState stmState = new STMState();
    public static CMDState cmdState = new CMDState();

    public static LEDState[] ledStates = new LEDState[]
            {
                    ledState0,
                    ledState1,
                    ledState2,
                    ledState3,
                    ledState4,
                    ledState5
            };

    static {
        ledState0.effect_index = 0;
        ledState1.effect_index = 0;
        ledState2.effect_index = 0;
        ledState3.effect_index = 0;
        ledState4.effect_index = 0;
        ledState5.effect_index = 0;

        ledState0.string_index = 0;
        ledState1.string_index = 1;
        ledState2.string_index = 2;
        ledState3.string_index = 3;
        ledState4.string_index = 4;
        ledState5.string_index = 5;
    }

    public static StateHandler[] handlers = new StateHandler[]{
            new StateHandler(ledState0, MessageChannel.LED0),
            new StateHandler(ledState1, MessageChannel.LED1),
            new StateHandler(ledState2, MessageChannel.LED2),
            new StateHandler(ledState3, MessageChannel.LED3),
            new StateHandler(ledState4, MessageChannel.LED4),
            new StateHandler(ledState5, MessageChannel.LED5),
            new StateHandler(stmState, MessageChannel.STM),
            new StateHandler(cmdState, MessageChannel.CMD),
    };

    static ByteBuffer dout;

    static NsdManager mNsdManager;

    static {
        dout = ByteBuffer.allocate(4096);
        dout.order(ByteOrder.LITTLE_ENDIAN);
    }

    static ConnectionListener udpListener = new ConnectionListener() {
        @Override
        public void onConnected() {
            Log.i(TAG, "onConnected");
            synchronized (sync) {
                connected = true;
            }
        }

        @Override
        public void onDisconnected() {
            Log.i(TAG, "onDisconnected");
            synchronized (sync) {
                connected = false;
            }
        }

        @Override
        public void onConnectionFailed(String errorMsg) {
            Log.i(TAG, "onConnectionFailed:" + errorMsg);
        }

        @Override
        public void onDataReceived(ByteBuffer data) {
            Log.i(TAG, "onDataReceived:" + data.limit());
            dout.clear();
            MessageHeader m = new MessageHeader();
            while (data.position() < data.limit()) {
                m.Unstash(data);
                Log.i(TAG, "Addr: " + m.address + ", Type: " + m.type);
                handlers[m.address].Handle(m, data, dout);
            }
        }
    };

    public static void sendObject(ByteBuffer b, int address, int version, IState state) {
        int c = b.position();
        MessageHeader m = new MessageHeader();
        m.address = (byte) address;
        m.size = (short) (m.Size() + state.Size());
        m.version = (short) version;
        m.type = MessageType.Data;
        m.Stash(b);
        state.Stash(b);
        c = b.position() - c;
        Log.i(TAG, "Sent " + c + " bytes to " + address);
    }

    public static void sendRequest(ByteBuffer b, int address) {
        MessageHeader m = new MessageHeader();
        m.address = (byte) address;
        m.size = m.Size();
        m.version = 0;
        m.type = MessageType.Req;
        m.Stash(b);
    }

    // commands are sent out of band for now
    public static void sendCommand(byte command, byte address) {
        ByteBuffer b = ByteBuffer.allocate(1024);
        MessageHeader m = new MessageHeader();
        m.address = address;
        m.size = m.Size();
        m.version = 0;
        m.type = MessageType.Cmd;
        m.Stash(b);
        b.put(command);
        udpService.send(b);
        Log.i(TAG, "SENT COMMAND: " + command);
    }

    void onEffectsClick() {
        Intent newIntent = new Intent(this, EffectActivity.class);
        newIntent.putExtra("layout", R.layout.activity_effect);
        startActivity(newIntent);
    }

    public void initializeResolveListener() {

        mResolveListener = new NsdManager.ResolveListener() {

            @Override
            public void onResolveFailed(NsdServiceInfo serviceInfo, int errorCode) {
                // Called when the resolve fails. Use the error code to debug.
                Log.e(TAG, "Resolve failed: " + errorCode);
            }

            @Override
            public void onServiceResolved(NsdServiceInfo serviceInfo) {
                Log.e(TAG, "Resolve Succeeded. " + serviceInfo);

                if (serviceInfo.getServiceName().equals(mServiceName)) {
                    Log.d(TAG, "Same IP.");
                    return;
                }
                NsdServiceInfo mService = serviceInfo;
                int port = mService.getPort();
                InetAddress host = mService.getHost();
            }
        };
    }

    public void initializeDiscoveryListener() {

        mDiscoveryListener = new NsdManager.DiscoveryListener() {

            // Called as soon as service discovery begins.
            @Override
            public void onDiscoveryStarted(String regType) {
                Log.d(TAG, "Service discovery started");
            }

            @Override
            public void onServiceFound(NsdServiceInfo service) {
                // A service was found! Do something with it.
                Log.d(TAG, "Service discovery success" + service);
                if (!service.getServiceType().equals(SERVICE_TYPE)) {
                    // Service type is the string containing the protocol and
                    // transport layer for this service.
                    Log.d(TAG, "Unknown Service Type: " + service.getServiceType());
                } else if (service.getServiceName().equals(mServiceName)) {
                    // The name of the service tells the user what they'd be
                    // connecting to. It could be "Bob's Chat App".
                    Log.d(TAG, "Same machine: " + mServiceName);
                } else if (service.getServiceName().contains("foo")){
                    mNsdManager.resolveService(service, mResolveListener);
                }
            }

            @Override
            public void onServiceLost(NsdServiceInfo service) {
                // When the network service is no longer available.
                // Internal bookkeeping code goes here.
                Log.e(TAG, "service lost: " + service);
            }

            @Override
            public void onDiscoveryStopped(String serviceType) {
                Log.i(TAG, "Discovery stopped: " + serviceType);
            }

            @Override
            public void onStartDiscoveryFailed(String serviceType, int errorCode) {
                Log.e(TAG, "Discovery failed: Error code:" + errorCode);
                mNsdManager.stopServiceDiscovery(this);
            }

            @Override
            public void onStopDiscoveryFailed(String serviceType, int errorCode) {
                Log.e(TAG, "Discovery failed: Error code:" + errorCode);
                mNsdManager.stopServiceDiscovery(this);
            }
        };
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        Bundle parameters = getIntent().getExtras();
        if (parameters != null && parameters.containsKey("layout")) {
            setContentView(parameters.getInt("layout"));

        } else {
            setContentView(R.layout.activity_main);
        }
        udpService.setConnectionListener(udpListener);
        udpService.connect("192.168.0.63", 9173);

        mNsdManager = (NsdManager)(getApplicationContext().getSystemService(Context.NSD_SERVICE));

        final Button effectsButton = findViewById(R.id.gotoButton);
        effectsButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                onEffectsClick();
            }
        });

        final Button button = findViewById(R.id.wifi_settings_button);
        button.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                startActivity(new Intent(Settings.ACTION_WIFI_SETTINGS));
            }
        });

        final Button saveButton = findViewById(R.id.saveButton);
        saveButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                sendCommand((byte) 1, MessageChannel.CMD);
            }
        });
    }
}
