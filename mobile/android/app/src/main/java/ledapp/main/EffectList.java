package ledapp.main;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.ImageView;
import android.widget.TextView;

public class EffectList extends BaseAdapter {

    private final Context mContext;
    private final String[] effects;

    public EffectList(Context context, String[] effects) {
        this.mContext = context;
        this.effects = effects;
    }

    @Override
    public int getCount() {
        return effects.length;
    }

    @Override
    public long getItemId(int position) {
        return 0;
    }

    @Override
    public Object getItem(int position) {
        return null;
    }

    @Override
    public View getView(int position, View convertView, ViewGroup parent) {

        final String effect = effects[position];

        if (convertView == null) {
            final LayoutInflater layoutInflater = LayoutInflater.from(mContext);
            convertView = layoutInflater.inflate(R.layout.linearlayout_effect, null);
        }

        final ImageView imageView = convertView.findViewById(R.id.imageview_effect);
        final TextView nameTextView = convertView.findViewById(R.id.textview_effect_name);

        nameTextView.setText(effect);
        // then setup image, depending on position
        return convertView;
    }
}
