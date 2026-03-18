#ifndef EDGEBOX_ESP_100_H
#define EDGEBOX_ESP_100_H
#include <SPI.h>
#include "RTClib.h"
#include <Adafruit_ADS1X15.h>

// early_init.c runs __attribute__((constructor(101))) before Arduino setup().
// This flag lets you verify at runtime that the early GPIO init executed.
#ifdef __cplusplus
extern "C" {
#endif
extern volatile bool early_init_ran;
#ifdef __cplusplus
}
#endif

// ###################### Digital OUTPUTS ######################
// ## NOTES!! ---> DO_GND or DO_24V NOT BOTH AT THE SAME TIME ## (DEPENDS ON DE VERSION)

#define DO_0         40  // DON'T USE THIS MOTHERFUCKER, ON BOOT 1SEG ON!!!!!!!!!!
#define DO_1         39
#define DO_2         38
#define DO_3         37
#define DO_4         36
#define DO_5         35

// ###################### Digital INPUTS #######################

#define DI_0         4
#define DI_1         5
#define DI_2         6
#define DI_3         7

// ###################### Analog OUTPUTS #######################

#define AO_0         42
#define AO_1         41

// ######################## RS-485 PINS ########################

#define RS_485_TX   17 
#define RS_485_RX   18
#define RS_485_RTS  8

// ########################## I2C PINS #########################

#define I2C_SCL     19 
#define I2C_SDA     20

// ########################## CAN PINS #########################

#define CAN_TXD     1 
#define CAN_RXD     2

class EdgeBox_ESP_100{

    // map outputs and inputs to class members for
    const uint8_t digital_outputs[6] = {DO_0, DO_1, DO_2, DO_3, DO_4, DO_5};
    const uint8_t digital_inputs[4] = {DI_0, DI_1, DI_2, DI_3};
    
    const uint8_t analog_outputs[2] = {AO_0, AO_1};

    private:
        void setupPinMode(uint8_t pin, gpio_mode_t mode,
                          gpio_pullup_t   pull_up   = GPIO_PULLUP_DISABLE,
                          gpio_pulldown_t pull_down = GPIO_PULLDOWN_DISABLE);
        bool isValidPin(uint8_t pin) const;
        bool isOutputPin(uint8_t pin) const;
        bool isInputPin(uint8_t pin) const;

    public:
        RTC_DS3231 rtc;
        Adafruit_ADS1115 analog_inputs;
        /**
         * Initializes the selected board peripherals.
         * @param initRTC          Initialize DS3231 RTC (default: true)
         * @param initAnalogInputs Initialize ADS1115 ADC (default: true)
         * Returns false if any enabled peripheral fails to respond on I²C.
         */
        bool init(bool initRTC = true, bool initAnalogInputs = true);

        /** Configures all digital outputs as OUTPUT and sets them LOW. */
        void setUpOutputs();

        /** Configures all digital inputs as INPUT (no pull). */
        void setUpInputs();

        /**
         * Overrides Arduino's global pinMode for EdgeBox pins.
         * Accepts the same Arduino constants: INPUT, OUTPUT,
         * INPUT_PULLUP, INPUT_PULLDOWN.
         * Adds pin-validation before configuring the GPIO.
         */
        void pinMode(uint8_t pin, uint8_t mode);

        /**
         * Overrides Arduino's global digitalWrite for EdgeBox output pins.
         * Uses gpio_set_level() directly — no Arduino HAL overhead.
         * Falls back to ::digitalWrite() for unknown pins.
         */
        void digitalWrite(uint8_t pin, uint8_t value);

        /**
         * Overrides Arduino's global digitalRead for EdgeBox input pins.
         * Uses gpio_get_level() directly — no Arduino HAL overhead.
         * Falls back to ::digitalRead() for unknown pins.
         */
        int digitalRead(uint8_t pin);

        /**
         * Reads an ADS1115 analog input channel with hardware compensation.
         * The EdgeBox-ESP-100 ADC inputs have different voltage dividers:
         *   Even channels (0, 2) → 10 kΩ  → no correction
         *   Odd  channels (1, 3) → 5.1 kΩ → raw value × 1.5
         * A fixed offset of 2708 LSB is subtracted to zero-reference the output.
         *
         * @param input  ADS1115 channel (0–3)
         * @return       Compensated raw ADC counts
         */
        int64_t readAnalogInput(uint8_t input);
        
#endif