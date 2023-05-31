# Assumes that ../pongoOS contains a clone of the latest pongoOS
# repository from https://github.com/checkra1n/pongoOS with
# compiled binaries.

brix.bin: main.c Makefile chip8.h chip8.c
	xcrun -sdk iphoneos gcc main.c chip8.c -o brix.bin -fno-stack-protector -arch arm64 -mabi=aapcs -Xlinker -kext -nostdlib -Xlinker -fatal_warnings -D_SECURE__STRING_H_ -O3

button_helper.bin: button_helper.c Makefile
	xcrun -sdk iphoneos gcc button_helper.c -o button_helper.bin -fno-stack-protector -arch arm64 -mabi=aapcs -Xlinker -kext -nostdlib -Xlinker -fatal_warnings -D_SECURE__STRING_H_ -O3

clean:
	rm -f brix.bin

start_pongoos_shell:
	make ../pongoOS/scripts
	./../pongoOS/scripts/pongoterm
