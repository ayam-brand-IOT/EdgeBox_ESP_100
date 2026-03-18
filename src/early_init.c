// early_init.c
// Runs before Arduino's setup() via __attribute__((constructor(101))).
// Priority 101 ensures the ESP-IDF GPIO driver is already up but Arduino
// has not yet executed any user code — safe to use gpio_config_t here.
#include "driver/gpio.h"

// All digital output pins of the EdgeBox-ESP-100.
// DO_0 (GPIO 40) has a ~1s boot glitch — this is exactly why we do this early.
#define DO_PIN_MASK  ((1ULL << 35) | (1ULL << 36) | (1ULL << 37) | \
                      (1ULL << 38) | (1ULL << 39) | (1ULL << 40))

volatile bool early_init_ran = false;  // flag compartido, consultable desde C++

void __attribute__((constructor(101), used)) early_gpio_init(void) {
    gpio_config_t io_conf = {};
    io_conf.intr_type    = GPIO_INTR_DISABLE;
    io_conf.mode         = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = DO_PIN_MASK;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en   = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf);

    // Force all outputs LOW immediately — prevents the DO_0 boot glitch
    for (int pin = 35; pin <= 40; pin++)
        gpio_set_level((gpio_num_t)pin, 0);

    early_init_ran = true;
}