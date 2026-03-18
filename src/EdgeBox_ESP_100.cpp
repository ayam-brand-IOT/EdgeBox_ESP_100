#include "EdgeBox_ESP_100.h"
#include "driver/gpio.h"   // gpio_mode_t, gpio_set_direction, gpio_set_pull_mode

// ---------------------------------------------------------------------------
// Private: atomic GPIO config via ESP-IDF gpio_config_t
// Configures direction, pull-up/down, and disables interrupts in one shot.
// Using gpio_config() instead of separate calls avoids intermediate states
// and glitches at boot time.
// ---------------------------------------------------------------------------
void EdgeBox_ESP_100::setupPinMode(uint8_t pin, gpio_mode_t mode,
                                   gpio_pullup_t   pull_up,
                                   gpio_pulldown_t pull_down) {
    gpio_config_t io_conf = {};
    io_conf.intr_type    = GPIO_INTR_DISABLE;
    io_conf.mode         = mode;
    io_conf.pin_bit_mask = (1ULL << pin);
    io_conf.pull_up_en   = pull_up;
    io_conf.pull_down_en = pull_down;
    gpio_config(&io_conf);
}

// ---------------------------------------------------------------------------
// Private: validate that the pin belongs to a known EdgeBox GPIO
// ---------------------------------------------------------------------------
bool EdgeBox_ESP_100::isValidPin(uint8_t pin) const {
    for (uint8_t p : digital_outputs) if (p == pin) return true;
    for (uint8_t p : digital_inputs)  if (p == pin) return true;
    for (uint8_t p : analog_outputs)  if (p == pin) return true;
    return false;
}

// ---------------------------------------------------------------------------
// Private: validate that the pin is a digital output of this board
// ---------------------------------------------------------------------------
bool EdgeBox_ESP_100::isOutputPin(uint8_t pin) const {
    for (uint8_t p : digital_outputs) if (p == pin) return true;
    return false;
}

// ---------------------------------------------------------------------------
// Private: validate that the pin is a digital input of this board
// ---------------------------------------------------------------------------
bool EdgeBox_ESP_100::isInputPin(uint8_t pin) const {
    for (uint8_t p : digital_inputs) if (p == pin) return true;
    return false;
}

// ---------------------------------------------------------------------------
// Public: drop-in replacement / override of Arduino's global pinMode()
// Accepts INPUT, OUTPUT, INPUT_PULLUP, INPUT_PULLDOWN
// ---------------------------------------------------------------------------
void EdgeBox_ESP_100::pinMode(uint8_t pin, uint8_t mode) {
    if (!isValidPin(pin)) {
        // Unknown pin – fall back to Arduino default and warn
        ::pinMode(pin, mode);
        return;
    }

    switch (mode) {
        case OUTPUT:
            setupPinMode(pin, GPIO_MODE_OUTPUT);
            break;
        case INPUT_PULLUP:
            setupPinMode(pin, GPIO_MODE_INPUT, GPIO_PULLUP_ENABLE, GPIO_PULLDOWN_DISABLE);
            break;
        case INPUT_PULLDOWN:
            setupPinMode(pin, GPIO_MODE_INPUT, GPIO_PULLUP_DISABLE, GPIO_PULLDOWN_ENABLE);
            break;
        case INPUT:
        default:
            setupPinMode(pin, GPIO_MODE_INPUT);
            break;
    }
}

// ---------------------------------------------------------------------------
// Public: drop-in replacement / override of Arduino's global digitalWrite()
// Uses gpio_set_level() — avoids Arduino HAL and is glitch-safe.
// Only applies to declared output pins; falls back to ::digitalWrite() otherwise.
// ---------------------------------------------------------------------------
void EdgeBox_ESP_100::digitalWrite(uint8_t pin, uint8_t value) {
    if (!isOutputPin(pin)) {
        ::digitalWrite(pin, value);
        return;
    }
    gpio_set_level((gpio_num_t)pin, value ? 1 : 0);
}

// ---------------------------------------------------------------------------
// Public: drop-in replacement / override of Arduino's global digitalRead()
// Uses gpio_get_level() — avoids Arduino HAL overhead.
// Falls back to ::digitalRead() for unknown pins.
// ---------------------------------------------------------------------------
int EdgeBox_ESP_100::digitalRead(uint8_t pin) {
    if (!isValidPin(pin)) return ::digitalRead(pin);
    return gpio_get_level((gpio_num_t)pin);
}

// --------------------------------------------------------------------------- with per-channel resistor compensation.
//
// Hardware note: the EdgeBox-ESP-100 uses different voltage divider resistors
// on each ADC channel:
//   Even channels (0, 2) → 10 kΩ  → reading is already correct
//   Odd  channels (1, 3) → 5.1 kΩ → under-reads; multiply by 1.5 to correct
//
// A fixed offset of 2708 LSB is subtracted to zero-reference the output.
// ---------------------------------------------------------------------------
int64_t EdgeBox_ESP_100::readAnalogInput(uint8_t input) {
    int64_t raw = analog_inputs.readADC_SingleEnded(input);
    // Odd channels have a 5.1 kΩ resistor instead of 10 kΩ → compensate
    if (input % 2 != 0) raw = static_cast<int64_t>(raw * 1.5f);
    return raw - 2708;
}

// ---------------------------------------------------------------------------

bool EdgeBox_ESP_100::init(bool initRTC, bool initAnalogInputs){
    bool ok = true;
    if (initRTC)          ok &= rtc.begin();
    if (initAnalogInputs) ok &= analog_inputs.begin();
    return ok;
}

void EdgeBox_ESP_100::setUpInputs(){
    for (uint8_t p : digital_inputs) {
        setupPinMode(p, GPIO_MODE_INPUT);
    }
}

void EdgeBox_ESP_100::setUpOutputs(){
    for (auto &output : digital_outputs) {
        setupPinMode(output, GPIO_MODE_OUTPUT);
        gpio_set_level((gpio_num_t)output, LOW); // Asegura que inicien en LOW
    }
}
