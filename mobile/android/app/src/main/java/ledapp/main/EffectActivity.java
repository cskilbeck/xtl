package ledapp.main;

import android.content.Intent;
import android.os.Bundle;
import android.support.v7.app.AppCompatActivity;
import android.util.Log;
import android.view.View;
import android.widget.AdapterView;
import android.widget.CheckBox;
import android.widget.GridView;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;

import ledapp.main.data.message.MessageChannel;

public class EffectActivity extends AppCompatActivity {

    final String TAG = "EffectActivity";

    final String[] effects = {
            "red",
            "green",
            "blue",
            "cyan",
            "magenta",
            "yellow",
            "orange",
            "white",
            "dim",
            "candy",
            "sine",
            "spark",
            "pulse",
            "solid",
            "flash",
            "sparkle",
            "twinkle",
            "glow"
    };

    int[] checkboxIds = new int[]{
            R.id.checkBox_0,
            R.id.checkBox_1,
            R.id.checkBox_2,
            R.id.checkBox_3,
            R.id.checkBox_4,
            R.id.checkBox_5
    };

    CheckBox[] checkbox = new CheckBox[6];

    public boolean onLongClick(View view, int effect_id) {
        Intent newIntent = new Intent(this, EditorActivity.class);
        newIntent.putExtra("effect", effect_id);
        newIntent.putExtra("effect_name", effects[effect_id]);
        startActivity(newIntent);
        return true;
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_effect);

        for (int i = 0; i < 6; ++i) {
            checkbox[i] = findViewById(checkboxIds[i]);
        }

        final GridView g = findViewById(R.id.effectsGridView);
        EffectList effectsAdapter = new EffectList(this, effects);
        g.setAdapter(effectsAdapter);

        final ByteBuffer b = ByteBuffer.allocate(4096);
        b.order(ByteOrder.LITTLE_ENDIAN);

        g.setOnItemClickListener(new AdapterView.OnItemClickListener() {
            @Override
            public void onItemClick(AdapterView<?> adapterView, View view, int effect, long l) {
                Log.i(TAG, "Clicked " + effect);
                if (MainActivity.connected) {
                    b.clear();
                    for (int i = 0; i < 6; ++i) {
                        if (checkbox[i].isChecked()) {
                            Log.i(TAG, "Sending " + i);
                            MainActivity.ledStates[i].effect_index = (byte) effect;
                            MainActivity.handlers[i].currentVersion += 1;
                            MainActivity.sendObject(b, MessageChannel.LED0 + i, MainActivity.handlers[i].currentVersion, MainActivity.handlers[i].obj);
                        }
                    }
                    if (b.position() > 0) {
                        MainActivity.udpService.send(b);
                    }
                }
            }
        });

        g.setOnItemLongClickListener(new AdapterView.OnItemLongClickListener() {
            @Override
            public boolean onItemLongClick(AdapterView<?> adapterView, View view, int i, long l) {
                return onLongClick(view, i);
            }
        });
    }
}
