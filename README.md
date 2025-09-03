# Build
```
cd build
cmake -DCMAKE_BUILD_TYPE=Debug  ..
make
```

# RUNNING

```
cp hello.uf2 /media/$USER/RPI-RP2/
sudo minicom -b 115200 -o -D /dev/ttyACM0
```

# Debug HID

When you install  `dmesg` will show something like:

`hid-generic 0003:16C0:048F.000F: input,hidraw0: USB HID v1.11 Device [Tormach Console Controller] on usb-0000:00:14.0-1/input0`

You can then push button to dump HID
`xxd /dev/hidraw0`
