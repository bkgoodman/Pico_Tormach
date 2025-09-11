
#ifndef TORMACH_H
#define TORMACH_H

#define HID_DATA_LEN 17
#define TORMACH_BUTTONS (9)
#define TORMACH_KNOBS (8)

typedef struct __attribute__((packed)) {
    uint8_t  buttons;                 // offset 0
    uint16_t knob[TORMACH_KNOBS];     // offset 1 (unaligned)
} tormach_data_t;

_Static_assert(offsetof(tormach_data_t, knob) == 1, "knob offset must be 1");


extern tormach_data_t hid_data;
#endif
