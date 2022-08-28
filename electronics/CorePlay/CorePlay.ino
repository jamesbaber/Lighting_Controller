#define MAX_DMX_CHANNELS 512
char buffer[MAX_DMX_CHANNELS*2+4];

void loop()
{
  // if somebody is sending data, better use it...
  if (Serial.available()) {
    emulate_enttec_dmx();
  }
}

byte hexdigit(char c)
{
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return 255;
}


int hex2bin(char *buf)
{
  byte b1, b2;
  int i=0, count=0;
  
  while (1) {
    b1 = hexdigit(buf[i++]);
    if (b1 > 15) break;
    b2 = hexdigit(buf[i++]);
    if (b2 > 15) break;
    buf[count++] = b1 * 16 + b2;
  }
  return count;
}

byte stringEndsWith(const char *str, const char *end)
{
  int i, len, elen;

  len = strlen(str);
  elen = strlen(end);
  if (len < elen) return 0;
  for (i=0; i < elen; i++) {
    char c1, c2;
    c1 = str[len - elen + i];
    c2 = end[i];
    if (c1 >= 'a' && c1 <= 'z') c1 = toupper(c1);
    if (c2 >= 'a' && c2 <= 'z') c2 = toupper(c2);
    if (c1 != c2) return 0;	
  }
  return 1;
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
    b = Serial.read();

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
          // display the first 5 channels on LEDs
          if (count >= 1) analogWrite(4, buffer[1]);
          if (count >= 2) analogWrite(5, buffer[2]);
          if (count >= 3) analogWrite(9, buffer[3]);
          if (count >= 4) analogWrite(15, buffer[4]);
          if (count >= 5) analogWrite(14, buffer[5]);
//Serial1.printf("%02X%02X%02X\r\n", buffer[3]&255, buffer[4]&255, buffer[5]&255);
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
