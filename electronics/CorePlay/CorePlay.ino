#define MAX_DMX_CHANNELS 512
char buffer[MAX_DMX_CHANNELS*2+4];

void loop()
{
  // if somebody is sending data, better use it...
  if (Serial.available()) {
    emulate_enttec_dmx();
  }
}
// emulate a subset of the Entec DMX USB Pro
//
#define STATE_START    0
#define STATE_LABEL    1
#define STATE_LEN_LSB  2
#define STATE_LEN_MSB  3
#define STATE_DATA     4
#define STATE_END      5

elapsedMillis timeout = 0;

void emulate_enttec_dmx(void)
{
  byte state = STATE_START;
  byte label;
  unsigned int index, count;
  byte b;

  while (1) {
    while (!Serial.available()) {
      // 0.5 seconds without data, reset state
      if (timeout > 500) state = STATE_START;
      // 20 seconds without data, revert to playing files
      if (timeout > 20000) return;
    }
    timeout = 0;
    b = Serial.read(); // read a single byte

    switch (state) {
      case STATE_START:
        if (b == 0x7E) state = STATE_LABEL;
        break;

      case STATE_LABEL:
        label = b;
        state = STATE_LEN_LSB;
        break;

      case STATE_LEN_LSB:
        count = b;
        state = STATE_LEN_MSB;
        break;

      case STATE_LEN_MSB:
        count |= (b << 8);
        index = 0;
        if (count > 0) {
          state = STATE_DATA;
        } else {
          state = STATE_END;
        }
        break;

      case STATE_DATA:
        if (index < sizeof(buffer)) {
          buffer[index++] = b;
        }
        count = count - 1;
        if (count == 0) state = STATE_END;
        break;

      case STATE_END:
        if (b == 0xE7 && label == 6 && index > 1) {
          count = index;
          if (count > MAX_DMX_CHANNELS) count = MAX_DMX_CHANNELS;
          
          for (index=1; index <= count; index++) {
            DmxSimple.write(index, buffer[index]);
          }
        }
      default:
        state = STATE_START;
        break;
    }
  }
}
