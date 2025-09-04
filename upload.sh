#!/bin/bash

# Helper script - quick upload

sudo stty -F /dev/ttyACM0 115200 cs8 cread clocal -ixon -ixoff; 
sleep 1
echo -e "\nbootsel\n" | sudo tee -a /dev/ttyACM0; 
sleep 3
sudo cp pico_tormach.uf2 /media/${USER}/RPI-RP2/

