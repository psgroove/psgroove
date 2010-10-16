teensy1=0
teensypp1=1
teensy2=2
teensypp2=3
at90usbkey=4
minimus1=5
minimus32=100
blackcat=6
xplain=7
olimex=8
usbtinymkii=9
bentio=10
openkubus=11

mcu[$teensy1]=at90usb162
board[$teensy1]=TEENSY
mhz_clock[$teensy1]=16
name[$teensy1]="Teensy 1.0"

mcu[$teensypp1]=at90usb646
board[$teensypp1]=TEENSY
mhz_clock[$teensypp1]=16
name[$teensypp1]="Teensy++ 1.0"

mcu[$teensy2]=atmega32u4
board[$teensy2]=TEENSY
mhz_clock[$teensy2]=16
name[$teensy2]="Teensy 2.0"

mcu[$teensypp2]=at90usb1286
board[$teensypp2]=TEENSY
mhz_clock[$teensypp2]=16
name[$teensypp2]="Teensy++ 2.0"

mcu[$at90usbkey]=at90usb1287
board[$at90usbkey]=USBKEY
mhz_clock[$at90usbkey]=8
name[$at90usbkey]="AT90USBKEY"

mcu[$minimus1]=at90usb162
board[$minimus1]=USBKEY
mhz_clock[$minimus1]=16
name[$minimus1]="Minimus v1"

mcu[$minimus32]=atmega32u2
board[$minimus32]=USBKEY
mhz_clock[$minimus32]=16
name[$minimus32]="Minimus 32"

mcu[$blackcat]=at90usb162
board[$blackcat]=BLACKCAT
mhz_clock[$blackcat]=16
name[$blackcat]="Blackcat"

mcu[$xplain]=at90usb1287
board[$xplain]=XPLAIN
mhz_clock[$xplain]=8
name[$xplain]="XPLAIN"

mcu[$olimex]=at90usb162
board[$olimex]=OLIMEX
mhz_clock[$olimex]=16
name[$olimex]="Olimex"

mcu[$usbtinymkii]=at90usb162
board[$usbtinymkii]=USBTINYMKII
mhz_clock[$usbtinymkii]=16
name[$usbtinymkii]="USBTINYMKII"

mcu[$bentio]=at90usb162
board[$bentio]=BENTIO
mhz_clock[$bentio]=16
name[$bentio]="Bentio"

mcu[$openkubus]=atmega16u4
board[$openkubus]=USBKEY
mhz_clock[$openkubus]=8
name[$openkubus]="OpenKubus"


while [ "x$1" != "x" ]; do
  targets="$targets ${!1}"
  shift
done
if [ "x$targets" == "x" ]; then
  for i in {0..11}; do
    targets="$targets $i"
  done
fi

rm -rf psgroove_hex/
mkdir psgroove_hex
make clean

for target in ${targets}; do
  for firmware in 3.01 3.10 3.15 3.41 ; do
    firmware=${firmware/./_}
    low_board=`echo ${board[$target]} | awk '{print tolower($0)}'`
    filename="psgroove_${low_board}_${mcu[$target]}_${mhz_clock[$target]}mhz_firmware_${firmware}"
    make TARGET=$filename MCU=${mcu[$target]} BOARD=${board[$target]} F_CPU=${mhz_clock[$target]}000000 FIRMWARE_VERSION=${firmware} || exit 1
    mkdir -p "psgroove_hex/${name[$target]}"
    mv *.hex "psgroove_hex/${name[$target]}/"
    make clean_list TARGET=$filename
  done
done

