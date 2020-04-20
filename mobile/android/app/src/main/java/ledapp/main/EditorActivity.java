package ledapp.main;

import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;

public class EditorActivity extends AppCompatActivity {

    final String TAG = "EditorActivity";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_editor);
        Log.i(TAG, "EditorActivity::OnCreate");
    }
}
