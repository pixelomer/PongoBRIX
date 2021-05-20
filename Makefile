# Assumes that ../pongoOS contains a clone of the latest pongoOS
# repository from https://github.com/checkra1n/pongoOS with
# compiled binaries.

brix.bin: main.c Makefile chip8.h chip8.c
	xcrun -sdk iphoneos gcc main.c chip8.c -o brix.bin -fno-stack-protector -arch arm64 -mabi=aapcs -Xlinker -kext -nostdlib -Xlinker -fatal_warnings -D_SECURE__STRING_H_ -O3

button_helper.bin: button_helper.c Makefile
	xcrun -sdk iphoneos gcc button_helper.c -o button_helper.bin -fno-stack-protector -arch arm64 -mabi=aapcs -Xlinker -kext -nostdlib -Xlinker -fatal_warnings -D_SECURE__STRING_H_ -O3

clean:
	rm -f brix.bin

load_brix: brix.bin
	python3.8 ../pongoOS/scripts/upload_data.py brix.bin
	(printf 'modload\n'; cat) | (../pongoOS/scripts/pongoterm || true)

load_buttons: button_helper.bin
	python3.8 ../pongoOS/scripts/upload_data.py button_helper.bin
	(printf 'modload\n'; cat) | (../pongoOS/scripts/pongoterm || true)