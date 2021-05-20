#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include "chip8.h"

// 4 >= nibble_no >= 1
// 2 >= byte_no >= 1
#define _NIBBLE(value, nibble_no) ((value & (0x000F << ((nibble_no-1)*4))) >> ((nibble_no-1)*4))
#define NIBBLE(value, nibble_no) _NIBBLE(value, 5-nibble_no)
#define BYTE(value, byte_no) ((value >> ((!(byte_no - 1)) * 8)) & 0x00FF)
#define OPCODE_VALUE(value) (value & 0x0FFF)

static uint8_t font[] = {
	// "0" character
	0b11110000,
	0b10010000,
	0b10010000,
	0b10010000,
	0b11110000,

	// "1" character
	0b00100000,
	0b01100000,
	0b00100000,
	0b00100000,
	0b01110000,

	// "2" character
	0b11110000,
	0b00010000,
	0b11110000,
	0b10000000,
	0b11110000,

	// "3" character
	0b11110000,
	0b00010000,
	0b11110000,
	0b00010000,
	0b11110000,

	// "4" character
	0b10010000,
	0b10010000,
	0b11110000,
	0b00010000,
	0b00010000,

	// "5" character
	0b11110000,
	0b10000000,
	0b11110000,
	0b00010000,
	0b11110000,

	// "6" character
	0b11110000,
	0b10000000,
	0b11110000,
	0b10010000,
	0b11110000,

	// "7" character
	0b11110000,
	0b00010000,
	0b00100000,
	0b01000000,
	0b01000000,

	// "8" character
	0b11110000,
	0b10010000,
	0b11110000,
	0b10010000,
	0b11110000,

	// "9" character
	0b11110000,
	0b10010000,
	0b11110000,
	0b00010000,
	0b11110000,

	// "A" character
	0b11110000,
	0b10010000,
	0b11110000,
	0b10010000,
	0b10010000,

	// "B" character
	0b11100000,
	0b10010000,
	0b11100000,
	0b10010000,
	0b11100000,

	// "C" character
	0b11110000,
	0b10000000,
	0b10000000,
	0b10000000,
	0b11110000,

	// "D" character
	0b11100000,
	0b10010000,
	0b10010000,
	0b10010000,
	0b11100000,

	// "E" character
	0b11110000,
	0b10000000,
	0b11110000,
	0b10000000,
	0b11110000,

	// "F" character
	0b11110000,
	0b10000000,
	0b11110000,
	0b10000000,
	0b10000000
};

static uint8_t is_little_endian = 2;

void chip8_perform_callback(chip8_t *self, chip8_event_t event) {
	chip8_callback_t callback = self->callbacks[event.type];
	if (callback) callback(self, event);
}

chip8_t *chip8_init(void) {
	chip8_t *self = malloc(sizeof(chip8_t));
	if (!self) return NULL;
	memset(self, 0, sizeof(*self));
	self->program_counter = 0x200;
	memcpy(&self->memory[0x50], font, sizeof(font));
	return self;
}

void chip8_cycle(chip8_t *self) {
	uint16_t opcode;
	{
		opcode = *(uint16_t *)&self->memory[self->program_counter];
		self->program_counter += 2;
		if (is_little_endian == 2) {
			uint16_t test = 0xABCD;
			is_little_endian = !!(((uint8_t *)&test)[0] == 0xCD);
		}
		if (is_little_endian) {
			uint8_t *opcode_pt = (uint8_t *)&opcode;
			uint8_t b2 = opcode_pt[1];
			opcode_pt[1] = opcode_pt[0];
			opcode_pt[0] = b2;
		}
	}
	uint16_t n;
	switch (NIBBLE(opcode, 1)) {
		case 0x0:
			n = NIBBLE(opcode, 4);
			if (n == 0x0) {
				// 00E0 (Display) - Clears the screen.
				uint8_t x, y;
				for (x=0; x < CHIP8_SCREEN_WIDTH; x++) {
					for (y=0; y < CHIP8_SCREEN_HEIGHT; y++) {
						self->framebuffer[x][y] = 0;
					}
				}
				if (self->callbacks[CHIP8_REDRAW]) {
					chip8_event_t event;
					event.type = CHIP8_REDRAW;
					event.redraw_event.x = 0;
					event.redraw_event.y = 0;
					event.redraw_event.width = CHIP8_SCREEN_WIDTH;
					event.redraw_event.height = CHIP8_SCREEN_HEIGHT;
					chip8_perform_callback(self, event);
				}
			}
			else if (n == 0xE) {
				// 00EE (Flow) - Returns from a subroutine.
				self->stack_pt--;
				if (self->stack_pt >= sizeof(self->stack)) self->stack_pt = (sizeof(self->stack) - 1);
				self->program_counter = self->stack[self->stack_pt];
			}
			break;
		case 0x1:
			// 1NNN (Flow) - Jumps to address NNN.
			self->program_counter = OPCODE_VALUE(opcode);
			break;
		case 0x2:
			// 2NNN (Flow) - Calls subroutine at NNN.
			self->stack[self->stack_pt++] = self->program_counter;
			if (self->stack_pt >= sizeof(self->stack)) self->stack_pt = 0x0;
			self->program_counter = OPCODE_VALUE(opcode);
			break;
		#define case(a, cond) case a: if (cond) self->program_counter += 2; break;
		// 3XNN (Cond) - Skips the next instruction if VX equals NN.
		case(0x3, self->registers[NIBBLE(opcode, 2)] == BYTE(opcode, 2));
		// 4XNN (Cond) - Skips the next instruction if VX doesn't equal NN.
		case(0x4, self->registers[NIBBLE(opcode, 2)] != BYTE(opcode, 2));
		// 5XY0 (Cond) - Skips the next instruction if VX equals VY.
		case(0x5, self->registers[NIBBLE(opcode, 2)] == self->registers[NIBBLE(opcode, 3)]);
		// 9XY0 (Cond) - Skips the next instruction if VX doesn't equal VY.
		case(0x9, self->registers[NIBBLE(opcode, 2)] != self->registers[NIBBLE(opcode, 3)]);
		#undef case
		case 0x6:
			// 6XNN (Const) - Sets VX to NN.
			self->registers[NIBBLE(opcode, 2)] = BYTE(opcode, 2);
			break;
		case 0x7:
			// 7XNN (Const) - Adds NN to VX. (Carry flag is not changed)
			self->registers[NIBBLE(opcode, 2)] += BYTE(opcode, 2);
			break;
		case 0x8:
			// 8XY? operations
			switch (NIBBLE(opcode, 4)) {
				#define case(a, op) case a: \
					self->registers[NIBBLE(opcode, 2)] = ( \
						self->registers[NIBBLE(opcode, 2)] op \
						self->registers[NIBBLE(opcode, 3)] \
					); \
					break;
				case(0x1, |); // 8XY1 (BitOp) - Sets VX to VX or VY. (Bitwise OR operation)
				case(0x2, &); // 8XY2 (BitOp) - Sets VX to VX and VY. (Bitwise AND operation)
				case(0x3, ^); // 8XY3 (BitOp) - Sets VX to VX xor VY.
				#undef case
				#define case(a,x,y,op) case a: { \
					uint16_t new_value = (uint16_t)self->registers[NIBBLE(opcode, x)] op (uint16_t)self->registers[NIBBLE(opcode, y)]; \
					self->registers[NIBBLE(opcode, x)] = self->registers[NIBBLE(opcode, x)] op self->registers[NIBBLE(opcode, y)]; \
					self->registers[0xF] = !(new_value > 0xFF); \
					break; \
				}
				case(0x4, 2, 3, +); // 8XY4 (Math) - Adds VY to VX.
				case(0x5, 2, 3, -); // 8XY5 (Math) - VY is subtracted from VX.
				case(0x7, 3, 2, -); // 8XY7 (Math) - Sets VX to VY minus VX.
				#undef case
				case 0x6:
					// 8XY6 (BitOp) - Stores the least significant bit of VX in VF and then shifts VX to the right by 1.
					self->registers[0xF] = self->registers[NIBBLE(opcode, 2)] & 0x01;
					self->registers[NIBBLE(opcode, 2)] >>= 1;
					break;
				case 0xE:
					// 8XYE (BitOp) - Stores the most significant bit of VX in VF and then shifts VX to the left by 1.
					self->registers[0xF] = !!(self->registers[NIBBLE(opcode, 2)] & (1<<7));
					self->registers[NIBBLE(opcode, 2)] <<= 1;
					break;
				case 0x0:
					// 8XY0 (Assign) - Sets VX to the value of VY.
					self->registers[NIBBLE(opcode, 2)] = self->registers[NIBBLE(opcode, 3)];
					break;
			}
			break;
		case 0xA:
			// ANNN (MEM) - Sets I to the address NNN.
			self->mem_pt = OPCODE_VALUE(opcode);
			break;
		case 0xB:
			// BNNN (Flow) - Jumps to the address NNN plus V0.
			self->program_counter = OPCODE_VALUE(opcode) + self->registers[0x0];
			break;
		case 0xC:
			// CXNN (Rand)
			// Sets VX to the result of a bitwise and operation on a               
			// random number (Typically: 0 to 255) and NN.
			self->registers[NIBBLE(opcode, 2)] = (rand() % 0x100) & BYTE(opcode, 2);
			break;
		case 0xD: {
			// DXYN (Disp)
			// Draws a sprite at coordinate (VX, VY) that has a width              
			// of 8 pixels and a height of N pixels. Each row of 8              
			// pixels is read as bit-coded starting from memory location              
			// I; I value doesn’t change after the execution of this              
			// instruction. VF is set to 1 if any screen pixels are flipped
			// from set to unset when the sprite is drawn, and to 0 if that
			// doesn’t happen.
			uint8_t height = NIBBLE(opcode, 4);
			uint8_t *lines = malloc(height);
			for (uint8_t i=0; i<height; i++) {
				if (i == 16) break;
				lines[i] = self->memory[(i + self->mem_pt) % 0x1000];
			}
			uint8_t x = self->registers[NIBBLE(opcode, 2)];
			uint8_t y = self->registers[NIBBLE(opcode, 3)];
			uint8_t xoffset = 0;
			uint8_t yoffset = 0;
			for (uint8_t yi=0; yi<height; yi++) {
				if ((yi+y-yoffset) >= CHIP8_SCREEN_HEIGHT) yoffset = CHIP8_SCREEN_HEIGHT;
				uint8_t line = lines[yi];
				xoffset = 0;
				for (uint8_t xi=0; xi<8; xi++) {
					if ((xi+x) >= CHIP8_SCREEN_WIDTH) xoffset = CHIP8_SCREEN_WIDTH;
					bool value = !!((line << xi) & (1 << 7));
					if (!value) continue;
					bool old_value = self->framebuffer[x+xi-xoffset][y+yi];
					self->registers[0xF] = old_value && value;
					self->framebuffer[x+xi-xoffset][y+yi] = (old_value && value) ? !value : value;
				}
			}
			free(lines);
			if (self->callbacks[CHIP8_REDRAW]) {
				chip8_event_t event;
				event.type = CHIP8_REDRAW;
				event.redraw_event.x = x;
				event.redraw_event.y = y;
				event.redraw_event.width = 8;
				event.redraw_event.height = height;
				chip8_perform_callback(self, event);
			}
			break;
		}
		case 0xE:
			switch (BYTE(opcode, 2)) {
				case 0x9E:
					// EX9E (KeyOp) - Skips the next instruction if the key stored in VX is pressed.
					if ((self->keyboard_mask << self->registers[NIBBLE(opcode, 2)]) & (1 << 15)) self->program_counter += 2;
					if (!self->dont_auto_update_keyboard_mask) self->keyboard_mask = 0;
					break;
				case 0xA1:
					// EXA1 (KeyOp) - Skips the next instruction if the key stored in VX isn't pressed.
					if (!((self->keyboard_mask << self->registers[NIBBLE(opcode, 2)]) & (1 << 15))) self->program_counter += 2;
					if (!self->dont_auto_update_keyboard_mask) self->keyboard_mask = 0;
					break;
			}
			break;
		case 0xF:
			switch (BYTE(opcode, 2)) {
				case 0x07:
					// FX07 (Timer) - Sets VX to the value of the delay timer.
					self->registers[NIBBLE(opcode, 2)] = self->timers.delay;
					break;
				case 0x0A: {
					// FX0A (KeyOp) - A key press is awaited, and then stored in VX. (Blocking Operation)
					uint16_t temp = self->keyboard_mask;
					if (!temp) self->program_counter-=2;
					else {
						uint8_t val = 0xF;
						while (!(temp & (1 << 15))) {
							temp <<= 1;
							val--;
						}
						self->registers[NIBBLE(opcode, 2)] = val;
						self->keyboard_mask = 0;
					}
					break;
				}
				case 0x15:
					// FX15 (Timer) - Sets the delay timer to VX.
					self->timers.delay = self->registers[NIBBLE(opcode, 2)];
					break;
				case 0x18:
					// FX18 (Sound) - Sets the sound timer to VX.
					self->timers.sound = self->registers[NIBBLE(opcode, 2)];
					break;
				case 0x1E: {
					// FX1E (MEM)
					// Adds VX to I. VF is set to 1 when there is a
					// range overflow (I+VX>0xFFF), and to 0 when there isn't.
					uint16_t new_value = self->mem_pt + self->registers[NIBBLE(opcode, 2)];
					self->mem_pt += self->registers[NIBBLE(opcode, 2)];
					self->mem_pt = self->mem_pt % 0x1000;
					self->registers[0xF] = new_value > 0xFFF;
					break;
				}
				case 0x29:
					// FX29 (MEM)
					// Sets I to the location of the sprite for the character in VX.
					// Characters 0-F (in hexadecimal) are represented by a 4x5 font.
					self->mem_pt = 0x50 + (self->registers[NIBBLE(opcode, 2)] * 5);
					break;
				case 0x33: {
					// FX33 (BCD)
					// Stores the binary-coded decimal representation of VX, with the
					// most significant of three digits at the address in I, the middle 
					// digit at I plus 1, and the least significant digit at I plus 2. 
					// (In other words, take the decimal representation of VX, place the
					// hundreds digit in memory at location in I, the tens digit at
					// location I+1, and the ones digit at location I+2.)
					uint8_t initial = self->registers[NIBBLE(opcode, 2)];
					uint8_t result, remainder;
					// Example: 153
					result = initial / 100;    // 153/100 = 1
					remainder = initial % 100; // 153%100 = 53
					self->memory[self->mem_pt % 0x1000] = result;
					result = remainder / 10;   // 53/10 = 5
					remainder = initial % 10;  // 153%10 = 3
					self->memory[(self->mem_pt+1) % 0x1000] = result;
					self->memory[(self->mem_pt+2) % 0x1000] = remainder;
					break;
				}
				#define FXN5_LEGACY_BEHAVIOR 0
				#if FXN5_LEGACY_BEHAVIOR
				#define NEXT(offset) self->mem_pt++
				#else
				#define NEXT(offset) ((self->mem_pt+offset) % 0x1000)
				#endif
				case 0x55: {
					// FX55 (MEM)
					// Stores V0 to VX (including VX) in memory starting at address I.
					// --------------------------------------------------------------
					// SCHIP ONLY: The offset from I is increased by 1 for each value 
					// written, but I itself is left unmodified.
					// --------------------------------------------------------------
					uint8_t register_index = NIBBLE(opcode, 2);
					for (uint8_t i=0; i < register_index+1; i++) {
						self->memory[NEXT(i)] = self->registers[i];
						#if FXN5_LEGACY_BEHAVIOR
						self->mem_pt = self->mem_pt % 0x1000;
						#endif
					}
					break;
				}
				case 0x65: {
					// FX65 (MEM)
					// Fills V0 to VX (including VX) with values from memory starting at
					// address I.
					// --------------------------------------------------------------
					// SCHIP ONLY: The offset from I is increased by 1 for each value 
					// written, but I itself is left unmodified.
					// --------------------------------------------------------------
					uint8_t register_index = NIBBLE(opcode, 2);
					for (uint8_t i=0; i < register_index+1; i++) {
						self->registers[i] = self->memory[NEXT(i)];
						#if FXN5_LEGACY_BEHAVIOR
						self->mem_pt = self->mem_pt % 0x1000;
						#endif
					}
					break;
				}
				#undef FXN5_LEGACY_BEHAVIOR
				#undef NEXT
			}
			break;
	}
	if (self->timers.delay > 0) self->timers.delay--;
	if (self->timers.sound > 0) self->timers.sound--;
	if (self->callbacks[CHIP8_CYCLE]) {
		chip8_event_t event;
		event.type = CHIP8_CYCLE;
		chip8_perform_callback(self, event);
	}
}

void chip8_keypress(chip8_t *self, uint8_t key) {
	if (key > 0xF) return;
	self->keyboard_mask |= (1 << key);
}

void chip8_free(chip8_t *self) {
	free(self);
}

void chip8_set_callback(chip8_t *self, chip8_callback_type_t type, chip8_callback_t callback) {
	if ((sizeof(self->callbacks) / sizeof(*(self->callbacks))) > type) {
		self->callbacks[type] = callback;
	}
}