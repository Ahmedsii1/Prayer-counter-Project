# Ultrasonic Proximity Event Counter

An Arduino Uno counts objects passing in front of an HC-SR04 ultrasonic sensor and shows the running total on a single-digit 7-segment display. Every **second** detection increments the digit — odd-numbered detections are deliberately discarded.

Built without a breadboard, without a display driver module, and without any external libraries.

---

## Table of contents

- [How it works](#how-it-works)
- [Bill of materials](#bill-of-materials)
- [Wiring](#wiring)
  - [HC-SR04](#hc-sr04)
  - [7-segment display](#7-segment-display)
  - [Why in-line resistors](#why-in-line-resistors)
- [Determining your own pin mapping](#determining-your-own-pin-mapping)
  - [Step 1 — common anode or common cathode](#step-1--common-anode-or-common-cathode)
  - [Step 2 — segment walk](#step-2--segment-walk)
  - [Step 3 — build the array](#step-3--build-the-array)
  - [Step 4 — upside-down mounting](#step-4--upside-down-mounting)
- [Main sketch](#main-sketch)
- [Configuration reference](#configuration-reference)
- [Serial output](#serial-output)
- [Test sketches](#test-sketches)
- [Troubleshooting](#troubleshooting)
- [Known limitations](#known-limitations)
- [Possible improvements](#possible-improvements)
- [License](#license)

---

## How it works

The sensor faces a gap that reads **50–60 cm** when empty. When an object enters the gap and the measured distance drops below **25 cm**, that registers as one *event*.

Events alternate:

| Event | Action |
|-------|--------|
| 1st | discarded |
| 2nd | display increments |
| 3rd | discarded |
| 4th | display increments |
| … | … |

After every event — counted or discarded — the sketch stops looking for new events and waits until the path is clear again. "Clear" means the distance has risen back above **30 cm** (the 25 cm trigger plus a 5 cm hysteresis margin). Only then does it resume searching.

That wait-for-clear behaviour guarantees **one event per object**. An object that enters and lingers in front of the sensor produces exactly one event, no matter how long it stays.

The display counts `0` through `9` and wraps back to `0`. Because only even events count, **the digit shows half the number of objects that actually passed** — a displayed `3` means six objects went by.

### Signal processing

Each measurement is the **median of three consecutive readings** taken 60 ms apart. A single HC-SR04 reading occasionally comes back garbage — a scattered echo, a late reflection — and the median discards it. Without this, phantom counts are common.

Every reading is **clamped to the range 2–70 cm**:

- Readings above 70 cm are reported as 70 cm.
- A timeout with no echo at all is also reported as 70 cm, since nothing reflecting means nothing is nearby.
- Readings below 2 cm are reported as 2 cm, because the HC-SR04 physically cannot resolve closer than that — the transducer is still ringing from its own burst.

Because every value is valid after clamping, the sketch has no error-handling path at all.

---

## Bill of materials

| Qty | Item | Notes |
|-----|------|-------|
| 1 | Arduino Uno (or any ATmega328P board) | Nano and Pro Mini work unchanged |
| 1 | HC-SR04 ultrasonic sensor | 5 V, 4-pin |
| 1 | Single-digit 7-segment display, 10-pin | e.g. 5161AS / 5161BS / 3161AS / 3161BS |
| 7 | 220 Ω resistor, ¼ W | 150 Ω–1 kΩ all work; higher = dimmer |
| 9 | Female-to-male DuPont jumper wires | 7 for segments, 2 for Trig/Echo |
| 2 | Male-to-male or female-to-male jumpers | 5 V and GND for the sensor |
| — | Heat shrink tubing, 2.5 mm | insulating the in-line resistor joints |

No breadboard, no shift register, no display driver IC.

---

## Wiring

### HC-SR04

| Sensor pin | Arduino pin |
|------------|-------------|
| VCC | 5V |
| GND | GND |
| Trig | **A0** |
| Echo | **A1** |

`A0` and `A1` are used here as **plain digital pins**, not analog inputs. Nothing reads a voltage level — `pinMode`, `digitalWrite` and `pulseIn` all accept them and treat them as digital I/O. On the Uno, `A0` is simply another name for digital pin 14.

**The sensor is deliberately on the analog side of the board.** It was originally on D9/D10, directly adjacent to the segment cluster on D2–D8. A single jumper off by one socket put Trig onto a segment line and the sensor stopped responding entirely. Moving it to A0/A1 puts physical distance between the two groups and removes that failure mode permanently. See [Troubleshooting](#troubleshooting).

### 7-segment display

**This build uses a COMMON ANODE display mounted UPSIDE DOWN** (decimal point at top left instead of bottom right).

| Connection | Goes to | Resistor |
|------------|---------|----------|
| Display common (pin 3 *or* pin 8) | **5V** | none |
| Each of the 7 segment pins | an Arduino digital pin | **220 Ω in series** |

Pins 3 and 8 are the same electrical node inside the package — they are bonded to the same common lead. Use whichever is mechanically convenient, or both. It makes no difference.

The segment-to-Arduino mapping for *this particular build* came out as:

| Visual position | Arduino pin |
|-----------------|-------------|
| a — top bar | D5 |
| b — upper right | D6 |
| c — lower right | D2 |
| d — bottom bar | D7 |
| e — lower left | D8 |
| f — upper left | D4 |
| g — middle bar | D3 |

> **This mapping is specific to this build's wiring and mounting orientation.** It encodes both a crossed set of jumpers and a 180° rotated display. If you rebuild this, your array will almost certainly differ. Follow [Determining your own pin mapping](#determining-your-own-pin-mapping) rather than copying the numbers.

#### Datasheet pinout, 10-pin package

Front view, decimal point at bottom right, pin 1 at bottom left:

```
        10   9   8   7   6
         g   f  com  a   b
         |   |   |   |   |
      +--------------------+
      |        --a--       |
      |       |     |      |
      |       f     b      |
      |       |     |      |
      |        --g--       |
      |       |     |      |
      |       e     c      |
      |       |     |      |
      |        --d--  o dp |
      +--------------------+
         |   |   |   |   |
         e   d  com  c   dp
         1   2   3   4   5
```

| Segment | Display pin |
|---------|-------------|
| a | 7 |
| b | 6 |
| c | 4 |
| d | 2 |
| e | 1 |
| f | 9 |
| g | 10 |
| dp | 5 |
| common | 3 and 8 |

### Why in-line resistors

Every segment needs its own current-limiting resistor. **Do not use a single resistor on the common pin.**

Two reasons:

1. With all seven segments lit at ~10 mA each, the common carries ~70 mA through one resistor. Segment brightness would then change depending on which digit is shown — an `8` would be dimmer than a `1`.
2. The current is pushed past the ATmega328P's limits.

And **do not connect segments to the Arduino with no resistor at all.** The arithmetic:

```
Red LED forward voltage        ≈ 1.9 V
ATmega328P driver resistance   ≈ 30 Ω
LED dynamic resistance         ≈ 20 Ω

I = (5 − 1.9) / (30 + 20) ≈ 62 mA per segment
```

The Uno's datasheet specifies 20 mA recommended, 40 mA absolute maximum per pin — so a bare segment runs at roughly **1.5× the absolute maximum**. Seven segments at once draws ~430 mA through the chip's VCC pin, which has a 200 mA absolute limit. The failure is cumulative rather than dramatic: output drivers degrade, pins start sourcing less current or stop working entirely.

#### Building resistor pigtails without a breadboard

1. Cut a female-to-male DuPont jumper in the middle.
2. Strip ~4 mm from both cut ends.
3. Solder (or tightly twist) a 220 Ω resistor between the two halves.
4. Slide 2.5 mm heat shrink over the resistor and shrink it.
5. Repeat 7 times.

The female end pushes onto the display's leg; the male end goes into the Arduino header.

**Alternative with no soldering:** bend one leg of a 220 Ω resistor 90° and push it directly into the Arduino's digital pin socket — the header accepts a resistor leg the same way it accepts a jumper. Twist the other leg onto a female jumper and insulate with tape.

**Alternative with less work:** a bussed SIP resistor network (8-pin, 220 Ω) handles all seven in one component.

---

## Determining your own pin mapping

Wire the seven segment lines to D2–D8 **in any order** — the order does not matter, because it gets resolved in software. Each line goes through a 220 Ω resistor. Connect the common pin to GND for now.

### Step 1 — common anode or common cathode

**Reading the part number.** The second-to-last character tells you:

| Suffix | Type | Common connects to |
|--------|------|--------------------|
| `...AS` (e.g. 5161**A**S) | common cathode | **GND** |
| `...BS` (e.g. 5161**B**S) | common anode | **5V** |

**With a multimeter.** Set it to diode mode. Black probe on display pin 3, red probe on display pin 7.

- Segment `a` lights → **common cathode**
- Only lights with probes swapped → **common anode**
- Neither direction → wrong pins, or a dead display

**By trial.** Upload the [segment walk](#step-2--segment-walk) sketch with `COMMON_CATHODE = true` and common on GND. If nothing lights at all, change **both** at once: set `COMMON_CATHODE = false` *and* move the common wire to 5V. One of the two combinations will work.

> Both changes must happen together. Flipping only the flag, or only the wire, leaves the display dark and is the most common source of confusion here. Neither combination can damage anything as long as the series resistors are in place.

### Step 2 — segment walk

This lights one Arduino pin at a time and prints which pin is active. Record which segment you actually see.

```cpp
/*
  Segment mapping walk.
  Serial Monitor at 9600 baud.
*/

const uint8_t PINS[7] = {2, 3, 4, 5, 6, 7, 8};
const bool COMMON_CATHODE = true;   // flip to false if nothing lights

void setup() {
  for (uint8_t i = 0; i < 7; i++) pinMode(PINS[i], OUTPUT);
  Serial.begin(9600);
  Serial.println(F("segment walk starting"));
}

void loop() {
  for (uint8_t i = 0; i < 7; i++) {
    for (uint8_t j = 0; j < 7; j++)
      digitalWrite(PINS[j], COMMON_CATHODE ? (i == j) : (i != j));
    Serial.print(F("now lit: D"));
    Serial.println(PINS[i]);
    delay(2000);
  }
  Serial.println(F("--- repeating ---"));
}
```

Segment letters:

```
      --a--
     |     |
     f     b
     |     |
      --g--
     |     |
     e     c
     |     |
      --d--   o dp
```

`a` top · `b` upper right · `c` lower right · `d` bottom · `e` lower left · `f` upper left · `g` middle.

> The pairs people misread are **b / f** (both upper verticals — `b` right, `f` left) and **c / e** (both lower verticals — `c` right, `e` left). Getting these wrong produces digits that look *almost* correct.

Fill in:

| Arduino pin | Segment seen |
|-------------|--------------|
| D2 | ? |
| D3 | ? |
| D4 | ? |
| D5 | ? |
| D6 | ? |
| D7 | ? |
| D8 | ? |

### Step 3 — build the array

`SEG_PINS` is read in the order **a, b, c, d, e, f, g**. So invert your table: for each letter in turn, write down which pin lit it.

Worked example. Suppose the walk produced:

| Pin | Segment |
|-----|---------|
| D2 | f |
| D3 | g |
| D4 | c |
| D5 | d |
| D6 | e |
| D7 | a |
| D8 | b |

Read it back by letter — a is on D7, b on D8, c on D4, d on D5, e on D6, f on D2, g on D3 — giving:

```cpp
const uint8_t SEG_PINS[7] = {7, 8, 4, 5, 6, 2, 3};  // a b c d e f g
```

Sanity check: every number 2–8 must appear **exactly once**. A repeated number means a letter was misread.

Verify with the [0–9 counter](#0-9-display-counter). A `1` should light the two bars on the **right** side. A `7` should be the top bar plus those two.

### Step 4 — upside-down mounting

If the display is physically rotated 180° (decimal point at top left instead of bottom right), no rewiring is needed. A 180° rotation maps each segment onto its opposite:

| What you want to see | Which physical segment is now there |
|----------------------|-------------------------------------|
| a (top) | old d |
| b (upper right) | old e |
| c (lower right) | old f |
| d (bottom) | old a |
| e (lower left) | old b |
| f (upper left) | old c |
| g (middle) | old g — unchanged |

Applying that to this build's `{7, 8, 4, 5, 6, 2, 3}` gives:

```cpp
const uint8_t SEG_PINS[7] = {5, 6, 2, 7, 8, 4, 3};  // a b c d e f g, rotated
```

The digit patterns in `DIGITS[]` stay exactly as they are. Only the array reorders.

> **The decimal point cannot be relocated in software.** There is only one dp on the die, and it stays at the top left when the display is inverted. This build does not use it.
>
> **Checking the rotation worked:** use a `7`, not a `2`. An upside-down `2` is still a plausible-looking `2`, so it gives a false pass. A correct `7` has the top bar and the right-hand vertical; an uncompensated one puts the bar at the bottom with the left vertical.

---

## Main sketch

Save as `proximity_counter.ino`. **No libraries to install** — `pulseIn()` and `digitalWrite()` are Arduino core functions.

```cpp
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
```

---

## Configuration reference

| Constant | Value | What it does |
|----------|-------|--------------|
| `SEG_PINS[7]` | `{5,6,2,7,8,4,3}` | Arduino pins in a-b-c-d-e-f-g order. **Setup-specific** — see [mapping](#determining-your-own-pin-mapping). |
| `TRIG_PIN` | `A0` | HC-SR04 trigger, used as a digital output |
| `ECHO_PIN` | `A1` | HC-SR04 echo, used as a digital input |
| `COMMON_CATHODE` | `false` | `false` = common anode (common on 5V). `true` = common cathode (common on GND). |
| `TRIGGER_CM` | `25` | Distance below which an event fires. Sits between the 20–30 cm event window and the 50–60 cm baseline. |
| `CLEAR_MARGIN_CM` | `5` | Hysteresis. Object must move past 30 cm before the next event can register. Raise if a slow object double-counts. |
| `MIN_VALID_CM` | `2` | Lower clamp. HC-SR04 hardware floor. |
| `MAX_VALID_CM` | `70` | Upper clamp. Anything further — including no echo — reports as 70. |
| `SETTLE_MS` | `250` | Pause after the path clears, before resuming. |
| `ECHO_TIMEOUT_US` | `5000` | `pulseIn` timeout. 70 cm round trip ≈ 4.1 ms, so 5 ms leaves margin. |

### Timing

A full measurement is three readings 60 ms apart, so roughly **200 ms per loop** when something is in range. With an empty path each reading times out after 5 ms instead of returning early, so the cycle stays around 195 ms.

Dropping `ECHO_TIMEOUT_US` from the original 25000 to 5000 made failed readings five times faster. There is no point waiting 25 ms for an echo that would be clamped to 70 anyway.

To make the system more responsive, reduce `medianDistanceCm()` from three readings to two — at the cost of weaker noise rejection.

---

## Serial output

Open the Serial Monitor at **9600 baud**.

```
distance: 56 cm            display: 0
distance: 58 cm            display: 0
distance: 14 cm            display: 0
event #1 -> discarded
waiting for the object to leave...
clear, resuming
distance: 57 cm            display: 0
distance: 11 cm            display: 0
event #2 -> counted, display = 1
waiting for the object to leave...
clear, resuming
distance: 70 cm (capped)   display: 1
```

The `(capped)` marker means the reading hit `MAX_VALID_CM` — either a genuinely distant object or no echo at all. The two are indistinguishable by design.

---

## Test sketches

### HC-SR04 bare test

Sensor only, nothing else. Reports the raw echo pulse width alongside the distance, which separates a wiring fault from a ranging problem.

```cpp
/*
  HC-SR04 bare test
  Trig -> A0,  Echo -> A1,  VCC -> 5V,  GND -> GND
  Serial Monitor at 9600 baud.
*/

const uint8_t TRIG_PIN = A0;
const uint8_t ECHO_PIN = A1;
const unsigned long ECHO_TIMEOUT_US = 30000UL;

void setup() {
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  digitalWrite(TRIG_PIN, LOW);
  Serial.begin(9600);
  delay(100);
  Serial.println(F("HC-SR04 test starting"));
}

void loop() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(4);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  unsigned long dur = pulseIn(ECHO_PIN, HIGH, ECHO_TIMEOUT_US);

  Serial.print(F("pulse: "));
  Serial.print(dur);
  Serial.print(F(" us   "));

  if (dur == 0) {
    Serial.println(F("NO ECHO - timeout"));
  } else {
    Serial.print(F("distance: "));
    Serial.print(dur * 0.0343 / 2.0, 1);
    Serial.println(F(" cm"));
  }

  delay(500);
}
```

Interpreting it:

| Output | Meaning |
|--------|---------|
| Every line `NO ECHO` | Echo pin never goes high — no power, Echo disconnected, or Trig not firing |
| Pulse pinned near 30000 with occasional real values | Sensor works but is aimed at nothing, or at a soft/angled surface that scatters the burst |
| Stable numbers | Sensor is fine; the fault is elsewhere |

### Trig continuity test

Holds Trig high for a second, low for a second. Put a multimeter on the **sensor's own Trig leg** and GND — you should watch it swing between ~5 V and ~0 V once per second.

```cpp
const uint8_t TRIG_PIN = A0;

void setup() { pinMode(TRIG_PIN, OUTPUT); }

void loop() {
  digitalWrite(TRIG_PIN, HIGH);
  delay(1000);
  digitalWrite(TRIG_PIN, LOW);
  delay(1000);
}
```

> Probe at the **sensor's pin header**, not at the Arduino socket. Testing at the Arduino only proves the Arduino works; testing at the sensor proves the wire between them works too. That distinction accounted for most of the debugging time on this build.

### 0–9 display counter

Display only, no sensor. Counts 0 through 9 with 300 ms between digits.

```cpp
/*
  7-segment counter 0-9, 300 ms per digit
*/

const uint8_t SEG_PINS[7] = {5, 6, 2, 7, 8, 4, 3};  // a b c d e f g
const bool COMMON_CATHODE = false;

const uint8_t DIGITS[10] = {
  0b0111111, 0b0000110, 0b1011011, 0b1001111, 0b1100110,
  0b1101101, 0b1111101, 0b0000111, 0b1111111, 0b1101111
};

void showDigit(uint8_t d) {
  uint8_t pattern = DIGITS[d % 10];
  for (uint8_t i = 0; i < 7; i++) {
    bool on = pattern & (1 << i);
    digitalWrite(SEG_PINS[i], COMMON_CATHODE ? on : !on);
  }
}

void setup() {
  for (uint8_t i = 0; i < 7; i++) pinMode(SEG_PINS[i], OUTPUT);
}

void loop() {
  for (uint8_t d = 0; d < 10; d++) {
    showDigit(d);
    delay(300);
  }
}
```

---

## Troubleshooting

### Nothing lights at all

Wrong display type. Change **both** `COMMON_CATHODE` *and* the physical common wire together:

- `true` ↔ common on GND
- `false` ↔ common on 5V

Half the failures come from flipping only one of the two.

If neither combination works, check continuity between the display's common pin and the Arduino pin it is supposed to reach. A DuPont female connector pushed onto a square display leg can look seated while only touching the plastic shroud — this was the single most frequent failure on this build.

### Some segments light, others do not

The display type and common connection are correct; individual lines are at fault.

1. **Measure each pigtail end to end.** Multimeter on resistance, one probe at each end of the jumper containing the resistor. Expect ~220 Ω. An open circuit means the twisted or soldered joint is sitting on insulation or oxidised copper.
2. **Swap test.** Move a dead segment's wire to a pin known to work and change `SEG_PINS` to match. Lights now → the original Arduino pin is damaged. Still dark → the wire or display leg is at fault.
3. **Wiggle test.** With the segment-walk sketch running, wiggle each connector. A segment that flickers on has found its own fault.

### All segments light, but in the wrong places

Crossed jumpers. Nothing needs rewiring — follow [Step 2](#step-2--segment-walk) and [Step 3](#step-3--build-the-array) and fix it in one line of code.

### Digits look almost right but a few are wrong

Misread letters during the segment walk — almost always `b`/`f` or `c`/`e`. Re-run the walk and pay attention to left versus right.

### Sensor stopped working after rewiring the display

**This is what happened on this build, and the diagnostic is worth reproducing.**

A multimeter between Trig and GND read a **steady 4.4 V**. That is the wrong signature entirely — Trig should sit near 0 V and pulse to 5 V for only 10 µs twice a second, which averages to single-digit millivolts on a meter. A constant 4.4 V means the pin is held high, not pulsed.

The cause was a pin collision. Segments occupied D2–D8 and the sensor sat on D9/D10, so D8 and D9 are adjacent in the header. A jumper off by one socket put Trig onto a segment line — and on a common-anode display, segments are driven **HIGH to stay off**, which produces exactly that steady ~4.4 V (the 0.6 V drop being the output driver under load).

**The fix:** move the sensor to `A0` and `A1`. Two lines of code, no rewiring of the display, and the adjacency problem disappears permanently.

```cpp
const uint8_t TRIG_PIN = A0;
const uint8_t ECHO_PIN = A1;
```

That relocating the pins fixed it also confirmed the diagnosis — a physically broken wire would not have been helped by changing which pin it plugs into.

### Sensor reads walls instead of the target

The HC-SR04's beam is roughly a 15° cone at the −3 dB point, but side lobes reach considerably wider and hard flat surfaces reflect strongly. A wall a metre off-axis can return a stronger echo than the intended target.

In rough order of effort:

1. **Re-aim.** Tilt the assembly 10–15° so the beam points away from the nearest wall. Free, and often sufficient.
2. **Shroud the transducers.** A 10–15 mm tube extending *past* the face of each can acts as a waveguide and cuts the side lobes. Line it with felt or foam if possible, and shroud the **receiver** especially. Beyond ~20 mm it gets worse, not better — internal reflections start showing up as phantom close readings. A 3D-printed two-hole collar is the highest-yield version of this.
3. **Gate in software.** `MAX_VALID_CM` already handles walls *beyond* 70 cm.
4. **Change sensor.** A VL53L0X IR time-of-flight sensor has a programmable ROI, letting you narrow the field of view in software, and runs over I²C on A4/A5. Tradeoff: IR is affected by sunlight and by dark or glossy targets, where ultrasonic is not.

> **None of the software options help if the wall is *nearer* than the target.** The HC-SR04 reports the first echo it receives, so a wall at 20 cm masks a target at 40 cm completely. That geometry needs the shroud or repositioning.

### The sketch appears frozen

`waitUntilClear()` blocks indefinitely while something sits within 30 cm. This is the intended behaviour, but a misaligned sensor pointed at a fixed obstacle looks identical to a dead board. The Serial Monitor makes it obvious — the "waiting for the object to leave" line repeats with no progress.

Add a `millis()` deadline inside that loop if you want a safety escape.

---

## Known limitations

- **Single digit.** The display wraps at 9, so the system cannot show more than 10 states. Counting past that needs a second digit and multiplexing.
- **Halved count.** Because only even events increment, the digit shows half the number of objects that passed. A `3` means six objects. Worth stating in any writeup, or anyone reading the display will misinterpret it.
- **Blocking design.** `delay()` and the blocking `waitUntilClear()` loop mean the sketch does nothing else while waiting. Fine for this scope; it would need rewriting around `millis()` to run alongside other tasks.
- **No distinction between "far" and "nothing".** Clamping at 70 cm means an empty room and a wall at exactly 70 cm produce identical readings. If the resting gap ever drifts above 70, the log pins at 70 with no warning that the sensor moved. Watch the first few minutes of Serial output after any physical change.
- **Count is not persistent.** A reset or power cycle returns the display to 0. Storing `count` in EEPROM would survive power loss.
- **Beam width.** See the wall-reading section above.

---

## Possible improvements

- Write `count` to EEPROM so it survives a reset.
- Add a momentary push button on a spare pin to reset the count to 0 manually.
- Replace the blocking structure with a `millis()`-based state machine.
- Add a second digit and multiplex, extending the range to 0–99.
- Swap the `eventCount % 2` test for a configurable divisor (`EVENT_DIVISOR`) so the discard ratio can be 1-in-3, 1-in-4, and so on.
- Drive the segments through a 74HC595 shift register, freeing five Arduino pins.
- Log each event with a timestamp over Serial for later analysis.

---

## License

MIT. Use it however you like.
