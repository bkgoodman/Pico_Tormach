#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include <common/tusb_common.h>
#include <stdio.h>
#include "tormach.h"

#define NUM_ENCODERS 3
#define ENCODER_COUNT_MIN 0
#define ENCODER_COUNT_MAX 999
// Each encoder: CLK, DT, SW
const uint8_t encoder_pins[NUM_ENCODERS][3] = {
    {2, 3, 4},   // Encoder 0
    {5, 16, 6},   // Encoder 1
    {7, 8, 9}    // Encoder 2
};

volatile int encoder_counts[NUM_ENCODERS] = {50};
volatile int prev_encoder_counts[NUM_ENCODERS] = {0};
volatile bool button_pressed[NUM_ENCODERS] = {false};
volatile uint8_t last_state[NUM_ENCODERS] = {0};

// Base scaling factors per encoder
// (can be adjusted at runtime)
volatile int encoder_scale[NUM_ENCODERS] = {10, 10, 10};

// Value when button pressed
const int button_press_value[NUM_ENCODERS] = {500,500,0};

// For timed debounce
static absolute_time_t last_time[NUM_ENCODERS] = {0};

// Quadrature transition lookup table
const int8_t transition_table[16] = {
    0,  -1,  +1,   0,
    +1,   0,   0,  -1,
    -1,   0,   0,  +1,
    0,  +1,  -1,   0
};

/// Host to Little-Endian Short (16-bit)
static inline uint16_t htols(uint16_t x)
{
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    return x;  // already little-endian
#else
    // swap bytes on big-endian systems
    return (x << 8) | (x >> 8);
#endif
}

void gpio_callback(uint gpio, uint32_t events) {
    gpio_put(PICO_DEFAULT_LED_PIN, !gpio_get(PICO_DEFAULT_LED_PIN));
    for (int i = 0; i < NUM_ENCODERS; i++) {
        uint clk = encoder_pins[i][0];
        uint dt  = encoder_pins[i][1];
        uint sw  = encoder_pins[i][2];

        // Handle encoder transitions
        if (gpio == clk || gpio == dt) {
            uint8_t new_state = (gpio_get(clk) << 1) | gpio_get(dt);
            uint8_t index = (last_state[i] << 2) | new_state;

            int8_t delta = transition_table[index];

            if (delta != 0) {
                int scale = encoder_scale[i];

                // Example: dynamic scaling for encoder 1
                if (i == 0) {  
                    if (encoder_counts[i] < 100) {
                        scale = 4;  // fine near zero
                    }
                }

                encoder_counts[i] += delta * scale;
#ifdef ENCODER_COUNT_MIN
		if (encoder_counts[i] < ENCODER_COUNT_MIN)
                	encoder_counts[i] = ENCODER_COUNT_MIN;
#endif
#ifdef ENCODER_COUNT_MAX
		if (encoder_counts[i] > ENCODER_COUNT_MAX)
                	encoder_counts[i] = ENCODER_COUNT_MAX;
#endif

            }

            last_state[i] = new_state;
        }

        // Handle button
        if (gpio == sw && (events & GPIO_IRQ_EDGE_FALL)) {
            absolute_time_t now = get_absolute_time();

            if (absolute_time_diff_us(last_time[i], now) > 200000) {
                last_time[i] = now;
		// We dont send press "events"
                //button_pressed[i] = true;
		// We just set values
		encoder_counts[i] = button_press_value[i];
            }
        }
    }
}

void knob_gpiodump() {
    for (int i = 0; i < NUM_ENCODERS; i++) {
        uint clk = encoder_pins[i][0];
        uint dt  = encoder_pins[i][1];
        uint sw  = encoder_pins[i][2];
	printf("Knob %d clk (pin %d) = %d dt (pin %d) = %d sw (pin %d) = %d\n",
			i,
			clk, gpio_get(clk),
			dt, gpio_get(dt),
			sw, gpio_get(sw));
    }
}

int knob_init() {
    // Init encoders
    for (int i = 0; i < NUM_ENCODERS; i++) {
        uint clk = encoder_pins[i][0];
        uint dt  = encoder_pins[i][1];
        uint sw  = encoder_pins[i][2];

        gpio_init(clk);
        gpio_set_dir(clk, GPIO_IN);
        //gpio_pull_up(clk);

        gpio_init(dt);
        gpio_set_dir(dt, GPIO_IN);
        //gpio_pull_up(dt);

        gpio_init(sw);
        gpio_set_dir(sw, GPIO_IN);
        //gpio_pull_up(sw);

        last_state[i] = (gpio_get(clk) << 1) | gpio_get(dt);
	last_time[i] = (absolute_time_t){0};
    }
    //gpio_set_irq_enabled_with_callback(0, 0, false, &gpio_callback);
    gpio_set_irq_enabled_with_callback(4, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &gpio_callback);
    for (int i = 0; i < NUM_ENCODERS; i++) {
	uint clk = encoder_pins[i][0];
        uint dt  = encoder_pins[i][1];
        uint sw  = encoder_pins[i][2];
        gpio_set_irq_enabled(clk, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
        gpio_set_irq_enabled(dt,  GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
        gpio_set_irq_enabled(sw,  GPIO_IRQ_EDGE_FALL, true);
    }

}

// Returns TRUE if it updated HID data
// (and caller should transmit)
bool knob_task() {
	bool update = false;
        for (int i = 0; i < NUM_ENCODERS; i++) {
	    if (encoder_counts[i] != prev_encoder_counts[i]) {
	    	prev_encoder_counts[i] = encoder_counts[i];
		hid_data.knob[i] = htols(prev_encoder_counts[i]);
		update = true;
            	printf("Encoder %d: %d (scale=%d)\n", i, encoder_counts[i], encoder_scale[i]);
	    }
            if (button_pressed[i]) {
                printf("Encoder %d button pressed!\n", i);
                button_pressed[i] = false;
		update = true;
            }
        }
	return update;
}

