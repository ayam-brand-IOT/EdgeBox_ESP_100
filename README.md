# EdgeBox_ESP_100

Arduino/PlatformIO library for the **[Seeed Studio EdgeBox-ESP-100](https://wiki.seeedstudio.com/Edge_Box_ESP_introduction/)** industrial edge controller.

The EdgeBox-ESP-100 is an ESP32-S3 based industrial automation gateway with:
- 6× digital outputs (DO_0–DO_5, sourcing or sinking, hardware-selectable)
- 4× digital inputs (DI_0–DI_3)
- 2× analog outputs (AO_0–AO_1)
- 2× analog inputs via ADS1115 (I²C)
- RS-485 interface
- CAN bus interface
- DS3231 RTC (I²C)

> ⚠️ **DO_0 (GPIO 40)** activates for ~1 second on boot due to ESP32-S3 boot strapping. Avoid using it for outputs that must be LOW at power-up.

---

## Installation

**PlatformIO** — add to `platformio.ini`:
```ini
lib_deps =
    https://github.com/elastra21/EdgeBox_ESP_100.git
```

**Arduino IDE** — download the ZIP and install via *Sketch → Include Library → Add .ZIP Library*.

---

## Pin Reference

| Group | Name | GPIO |
|---|---|---|
| Digital Output | DO_0 | 40 ⚠️ boot glitch |
| Digital Output | DO_1 | 39 |
| Digital Output | DO_2 | 38 |
| Digital Output | DO_3 | 37 |
| Digital Output | DO_4 | 36 |
| Digital Output | DO_5 | 35 |
| Digital Input | DI_0 | 4 |
| Digital Input | DI_1 | 5 |
| Digital Input | DI_2 | 6 |
| Digital Input | DI_3 | 7 |
| Analog Output | AO_0 | 42 |
| Analog Output | AO_1 | 41 |
| RS-485 TX | RS_485_TX | 17 |
| RS-485 RX | RS_485_RX | 18 |
| RS-485 RTS | RS_485_RTS | 8 |
| I²C SCL | I2C_SCL | 19 |
| I²C SDA | I2C_SDA | 20 |
| CAN TX | CAN_TXD | 1 |
| CAN RX | CAN_RXD | 2 |

## Early Boot GPIO Init

The library includes `early_init.c` which runs **before Arduino's `setup()`** using `__attribute__((constructor(101)))`. This directly solves the DO_0 (GPIO 40) ~1s HIGH glitch caused by ESP32-S3 boot strapping.

```
[ ESP-IDF driver init ]
        ↓
[ constructor(101) → early_gpio_init() ]  ← all DOs forced LOW here
        ↓
[ Arduino init ]
        ↓
[ setup() ]
```

All 6 digital outputs are configured with `gpio_config_t` and set to `LOW` atomically before any user code runs. You can verify it executed via the shared flag:

```cpp
void setup() {
    if (!early_init_ran) {
        Serial.println("WARNING: early GPIO init did not run!");
    }
}
```

---

## API

### `bool init(bool initRTC = true, bool initAnalogInputs = true)`
Initializes the selected board peripherals. Returns `false` if any enabled peripheral fails to respond on I²C.

```cpp
edgebox.init();                        // both RTC + ADC (default)
edgebox.init(true, false);             // RTC only
edgebox.init(false, true);             // ADC only
edgebox.init(false, false);            // neither — GPIO only

if (!edgebox.init()) {
    Serial.println("ERROR: RTC or ADC not responding");
    while (true);
}
```

### `void setUpOutputs()`
Configures all 6 digital outputs using the ESP-IDF `gpio_config_t` API and sets them all LOW.

### `void setUpInputs()`
Configures all 4 digital inputs using the ESP-IDF `gpio_config_t` API (no pull resistors by default — use `pinMode()` individually if you need pull-up/down on specific channels).

### `void pinMode(uint8_t pin, uint8_t mode)`
Drop-in override of Arduino's global `pinMode()`.  
- Uses `gpio_config_t` internally — single atomic call to the ESP-IDF driver, avoiding intermediate states and boot glitches.
- Accepts standard Arduino constants: `INPUT`, `OUTPUT`, `INPUT_PULLUP`, `INPUT_PULLDOWN`.
- For unknown pins (not part of the EdgeBox pinout), falls back to `::pinMode()`.

```cpp
edgebox.pinMode(DO_1, OUTPUT);
edgebox.pinMode(DI_2, INPUT_PULLUP);
```

### `void digitalWrite(uint8_t pin, uint8_t value)`
Drop-in override of Arduino's global `digitalWrite()`.  
- Uses `gpio_set_level()` directly — bypasses the Arduino HAL layer.
- Only applies to declared digital output pins (DO_0–DO_5). Falls back to `::digitalWrite()` for any other pin.

```cpp
edgebox.digitalWrite(DO_1, HIGH);
edgebox.digitalWrite(DO_3, LOW);
```

### `int digitalRead(uint8_t pin)`
Drop-in override of Arduino's global `digitalRead()`.
- Uses `gpio_get_level()` directly.
- Applies to any declared EdgeBox pin (inputs and outputs). Falls back to `::digitalRead()` for unknown pins.

```cpp
int state = edgebox.digitalRead(DI_0); // 0 or 1
```

### `int64_t readAnalogInput(uint8_t input)`
Reads an ADS1115 channel with automatic per-channel hardware compensation.

> **Hardware quirk:** The EdgeBox-ESP-100 uses different voltage divider resistors on each ADC channel — a bad design decision that requires software correction:
>
> | Channel | Resistor | Correction |
> |---|---|---|
> | 0, 2 | 10 kΩ | none |
> | 1, 3 | 5.1 kΩ | raw × 1.5 |
>
> A fixed offset of **2708 LSB** is also subtracted to zero-reference the output.

```cpp
int64_t ch0 = edgebox.readAnalogInput(0); // 10 kΩ channel, no correction
int64_t ch1 = edgebox.readAnalogInput(1); // 5.1 kΩ channel, raw × 1.5
int64_t ch2 = edgebox.readAnalogInput(2);
int64_t ch3 = edgebox.readAnalogInput(3);
```

### `RTC_DS3231 rtc`
Public member. Access the onboard RTC directly:
```cpp
edgebox.rtc.begin();
DateTime now = edgebox.rtc.now();
```

### `Adafruit_ADS1115 analog_inputs`
Public member. Gives direct access to the ADS1115 driver if you need low-level control.
For normal use, prefer `readAnalogInput()` which handles the resistor compensation automatically.

```cpp
// Low-level direct access (no compensation applied)
int16_t raw = edgebox.analog_inputs.readADC_SingleEnded(0);
```

---

## Basic Example

```cpp
#include <EdgeBox_ESP_100.h>

EdgeBox_ESP_100 edgebox;

void setup() {
    Serial.begin(115200);

    if (!edgebox.init()) {   // init RTC + ADS1115, check for faults
        Serial.println("Hardware init failed!");
        while (true);
    }
    edgebox.setUpOutputs();  // all DOs → OUTPUT + LOW
    edgebox.setUpInputs();   // all DIs → INPUT (floating)

    edgebox.pinMode(DI_0, INPUT_PULLUP); // override specific input if needed
}

void loop() {
    // Read digital input and mirror to output
    bool btn = (digitalRead(DI_0) == LOW);
    edgebox.digitalWrite(DO_1, btn ? HIGH : LOW);

    // Read analog inputs (compensated for resistor mismatch)
    int64_t ch0 = edgebox.readAnalogInput(0); // 10 kΩ channel
    int64_t ch1 = edgebox.readAnalogInput(1); // 5.1 kΩ → ×1.5 corrected

    Serial.printf("AI0: %lld  AI1: %lld\n", ch0, ch1);
    delay(100);
}
```

---

## Dependencies

| Library | Purpose |
|---|---|
| [RTClib](https://github.com/adafruit/RTClib) | DS3231 RTC |
| [Adafruit ADS1X15](https://github.com/adafruit/Adafruit_ADS1X15) | ADS1115 analog inputs |

---

## Why ESP-IDF GPIO instead of Arduino HAL?

The EdgeBox-ESP-100 occasionally shows output glitches during boot when using Arduino's `pinMode` / `digitalWrite` (which go through multiple driver layers). This library uses the ESP-IDF `gpio_config_t` / `gpio_set_level()` API directly:

- **`gpio_config_t`** — configures direction, pull-up/down, and interrupt mode in a single atomic call, eliminating intermediate pin states.
- **`gpio_set_level()`** — writes the output register directly with no HAL overhead.

For unknown pins the library transparently falls back to the standard Arduino functions, so it works alongside existing Arduino code without changes.

---

## License

MIT — see [license.md](license.md)

## Author

Emmanuel Lastra Williams — [@elastra21](https://github.com/elastra21)