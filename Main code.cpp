/*
  Proximity event counter
  -----------------------
  HC-SR04 watches a gap that normally reads 50-60 cm.
  When something enters and the distance drops below TRIGGER_CM,
  that counts as one EVENT. Events alternate:

      event 1 -> discarded
      event 2 -> display increments
      event 3 -> discarded
      event 4 -> display increments   ... and so on

  After every event the sketch stops counting until the object leaves
  (distance rises above TRIGGER_CM + CLEAR_MARGIN_CM). Then it starts
  searching again.

  DISTANCE CLAMPING
  Every reading is clamped into the range MIN_VALID_CM..MAX_VALID_CM.
  Anything measured beyond 70 cm is reported as 70 cm, and so is a
  timeout with no echo at all. The sketch therefore never has to deal
  with an invalid reading, and distant walls beyond 70 cm can no longer
  influence anything.

  Display counts 0..9 and wraps back to 0 after 9.

  No external libraries required.

  --- WIRING ---
  Display is COMMON ANODE: display common pin -> 5V, no resistor
  220 ohm resistor in series with EACH segment line.

  The display is mounted UPSIDE DOWN (decimal point at top left).
  The wiring is unchanged; SEG_PINS below is reordered to compensate.

      visual a (top)          -> D5
      visual b (upper right)  -> D6
      visual c (lower right)  -> D2
      visual d (bottom)       -> D7
      visual e (lower left)   -> D8
      visual f (upper left)   -> D4
      visual g (middle)       -> D3

  HC-SR04:  VCC -> 5V,  GND -> GND,  Trig -> A0,  Echo -> A1
*/

// ---------------- configuration ----------------

const uint8_t SEG_PINS[7] = {5, 6, 2, 7, 8, 4, 3};  // order: a b c d e f g
const uint8_t TRIG_PIN = A0;
const uint8_t ECHO_PIN = A1;

const bool COMMON_CATHODE = false;  // this display is common anode

const int TRIGGER_CM = 25;          // below this = special case
const int CLEAR_MARGIN_CM = 5;      // must rise past 30 cm to count as clear

const int MIN_VALID_CM = 2;         // HC-SR04 cannot resolve closer than this
const int MAX_VALID_CM = 70;        // anything further is reported as 70

const unsigned long SETTLE_MS = 250;

// 70 cm round trip is about 4.1 ms; 5 ms leaves margin and keeps the
// loop responsive when nothing is in range.
const unsigned long ECHO_TIMEOUT_US = 5000UL;

// ---------------- segment patterns ----------------
// bit 0 = a, bit 1 = b, ... bit 6 = g

const uint8_t DIGITS[10] = {
  0b0111111,  // 0
  0b0000110,  // 1
  0b1011011,  // 2
  0b1001111,  // 3
  0b1100110,  // 4
  0b1101101,  // 5
  0b1111101,  // 6
  0b0000111,  // 7
  0b1111111,  // 8
  0b1101111   // 9
};

uint8_t count = 0;            // what the display shows
unsigned int eventCount = 0;  // total detections so far

// ---------------- display ----------------

void showDigit(uint8_t d) {
  uint8_t pattern = DIGITS[d % 10];
  for (uint8_t i = 0; i < 7; i++) {
    bool on = pattern & (1 << i);
    digitalWrite(SEG_PINS[i], COMMON_CATHODE ? on : !on);
  }
}

// ---------------- distance ----------------

// Always returns a value between MIN_VALID_CM and MAX_VALID_CM.
// A timeout (no echo) is reported as MAX_VALID_CM, since nothing
// reflecting means nothing is nearby.
long readDistanceCm() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(4);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  unsigned long dur = pulseIn(ECHO_PIN, HIGH, ECHO_TIMEOUT_US);
  if (dur == 0) return MAX_VALID_CM;        // no echo -> treat as far

  long cm = (long)(dur * 0.0343 / 2.0);     // 343 m/s, round trip

  if (cm > MAX_VALID_CM) cm = MAX_VALID_CM; // clamp far
  if (cm < MIN_VALID_CM) cm = MIN_VALID_CM; // clamp near
  return cm;
}

// Median of three readings, rejects the occasional bad echo.
long medianDistanceCm() {
  long v[3];

  for (uint8_t i = 0; i < 3; i++) {
    v[i] = readDistanceCm();
    delay(60);                              // HC-SR04 needs settling time
  }

  for (uint8_t i = 1; i < 3; i++) {         // insertion sort
    for (uint8_t j = i; j > 0 && v[j - 1] > v[j]; j--) {
      long t = v[j]; v[j] = v[j - 1]; v[j - 1] = t;
    }
  }
  return v[1];                              // middle of three
}

// Blocks until the path is clear again.
void waitUntilClear() {
  Serial.println(F("waiting for the object to leave..."));
  while (medianDistanceCm() <= TRIGGER_CM + CLEAR_MARGIN_CM) {
    // keep polling
  }
  Serial.println(F("clear, resuming"));
  delay(SETTLE_MS);
}

// ---------------- main ----------------

void setup() {
  for (uint8_t i = 0; i < 7; i++) pinMode(SEG_PINS[i], OUTPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  digitalWrite(TRIG_PIN, LOW);

  Serial.begin(9600);
  showDigit(count);                         // start at 0
}

void loop() {
  long d = medianDistanceCm();

  Serial.print(F("distance: "));
  Serial.print(d);
  if (d >= MAX_VALID_CM) Serial.print(F(" cm (capped)"));
  else                   Serial.print(F(" cm         "));
  Serial.print(F("   display: "));
  Serial.println(count);

  if (d < TRIGGER_CM) {
    eventCount++;

    Serial.print(F("event #"));
    Serial.print(eventCount);

    if (eventCount % 2 == 0) {              // 2nd, 4th, 6th ... event counts
      count = (count + 1) % 10;
      showDigit(count);
      Serial.print(F(" -> counted, display = "));
      Serial.println(count);
    } else {                                // 1st, 3rd, 5th ... discarded
      Serial.println(F(" -> discarded"));
    }

    waitUntilClear();
  }
}