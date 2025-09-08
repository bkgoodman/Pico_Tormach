
#ifndef TORMACH_H
#define TORMACH_H

#define HID_DATA_LEN 17
#define TORMACH_BUTTONS (9)
#define TORMACH_KNOBS (8)
typedef struct tormach_data_s {
  uint8_t buttons; // 8 buttons
  uint16_t knob[TORMACH_KNOBS]; // I think there are really 9 (according to HID descriptor)
}  __attribute__ ((aligned(2))) *tormach_data_p, tormach_data_t;

extern tormach_data_t hid_data;
#endif
