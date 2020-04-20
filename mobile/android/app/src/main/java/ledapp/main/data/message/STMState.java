package ledapp.main.data.message;

import java.nio.ByteBuffer;

import ledapp.main.data.IState;

public class STMState implements IState {
    public short spi_errors;
    public short adc_value;
    public short frame_count;
    public short pad;

    @Override
    public void Stash(ByteBuffer buffer) {
        buffer.putShort(spi_errors);
        buffer.putShort(adc_value);
        buffer.putShort(frame_count);
        buffer.putShort(pad);
    }

    @Override
    public void Unstash(ByteBuffer buffer) {
        spi_errors = buffer.getShort();
        adc_value = buffer.getShort();
        frame_count = buffer.getShort();
        pad = buffer.getShort();
    }

    @Override
    public short Size() {
        return 8;
    }
}
