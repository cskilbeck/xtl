package ledapp.main.data.message;

import java.nio.ByteBuffer;

import ledapp.main.data.IState;
import ledapp.main.data.params.ColorRange;
import ledapp.main.data.params.SpeedRange;

public class LEDState implements IState {

    static final int max_color_ranges = 8;
    static final int max_speed_ranges = 8;

    /**
     * Which LED string this state represents
     */
    public byte string_index;

    /**
     * current effect playing on this LED string
     */
    public byte effect_index;

    /**
     * all the ColorRange parameters for the string
     */
    public ColorRange[] color_ranges = new ColorRange[max_color_ranges];

    /**
     * the SpeedRange parameters for the string
     */
    public SpeedRange[] speed_ranges = new SpeedRange[max_speed_ranges];

    public LEDState() {
        for (int i = 0; i < max_color_ranges; ++i) {
            color_ranges[i] = new ColorRange();
        }

        for (int i = 0; i < max_speed_ranges; ++i) {
            speed_ranges[i] = new SpeedRange();
        }
    }

    @Override
    public void Stash(ByteBuffer buffer) {
        buffer.put(string_index);
        buffer.put(effect_index);
        for (ColorRange r : color_ranges) r.Stash(buffer);
        for (SpeedRange s : speed_ranges) s.Stash(buffer);
    }

    @Override
    public void Unstash(ByteBuffer buffer) {
        string_index = buffer.get();
        effect_index = buffer.get();
        for (ColorRange r : color_ranges) r.Unstash(buffer);
        for (SpeedRange s : speed_ranges) s.Unstash(buffer);
    }

    @Override
    public short Size() {
        return (short) (2 + color_ranges[0].Size() * max_color_ranges + speed_ranges[0].Size() * max_speed_ranges);
    }
}
