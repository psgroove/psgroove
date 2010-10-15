teensy1=0
teensypp1=1
teensy2=2
teensypp2=3
at90usbkey=4
minimus=5

mcu[$teensy1]=at90usb162
board[$teensy1]=TEENSY
mhz_clock[$teensy1]=16

mcu[$teensypp1]=at90usb646
board[$teensypp1]=TEENSY
mhz_clock[$teensypp1]=16

mcu[$teensy2]=atmega32u4
board[$teensy2]=TEENSY
mhz_clock[$teensy2]=16

mcu[$teensypp2]=at90usb1286
board[$teensypp2]=TEENSY
mhz_clock[$teensypp2]=16

mcu[$at90usbkey]=at90usb1287
board[$at90usbkey]=USBKEY
mhz_clock[$at90usbkey]=8

mcu[$minimus]=at90usb162
board[$minimus]=USBKEY
mhz_clock[$minimus]=16

rm -rf hex/
mkdir hex
make clean

for target in {0..5}; do
  for firmware in 3.01 3.10 3.15 3.41 ; do
    firmware=${firmware/./_}
    low_board=`echo ${board[$target]} | awk '{print tolower($0)}'`
    filename="psgroove_${low_board}_${mcu[$target]}_${mhz_clock[$target]}mhz_firmware_${firmware}"
    make TARGET=$filename MCU=${mcu[$target]} BOARD=${board[$target]} F_CPU=${mhz_clock[$target]}000000 FIRMWARE_VERSION=${firmware} || exit 1
    mv *.hex hex
    make clean_list TARGET=$filename
  done
done

