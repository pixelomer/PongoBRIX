#include <stdio.h>
#include <stdlib.h>
#include "chip8.h"
#include "pongo.h"

#define MIN(a,b) ((a>b)?b:a)

struct device_model {
	const char *name;
	const volatile uint32_t *volume_up;
	const volatile uint32_t *volume_down;
	const volatile uint32_t *mute_switch;
	const volatile uint32_t *power_button;
	const volatile uint32_t *home_button;
	uint8_t home_button_at_right;
};

const struct device_model devices[] = {
	{
		.name = "iPhone10,6",
		.volume_up = 0x233100098,
		.volume_down = 0x2331000ec,
		.mute_switch = NULL,
		.power_button = 0x2331000e8,
		.home_button = NULL,
		.home_button_at_right = 0
	},
	{
		.name = "iPhone6,2",
		.volume_up = 0x20e300010,
		.volume_down = 0x20e300014,
		.mute_switch = 0x20e300040,
		.power_button = 0x20e30000c,
		.home_button = 0x20e300008,
		.home_button_at_right = 1
	},
	{
		.name = "iPhone7,2",
		.volume_up = 0x20e3000b4,
		.volume_down = 0x20e3000b8,
		.mute_switch = 0x20e30020c,
		.power_button = 0x20e300084,
		.home_button = 0x20e300080,
		.home_button_at_right = 1
	},
	{
		.name = "iPhone8,1",
		.volume_up = 0x20f100108,
		.volume_down = 0x20f10010c,
		.mute_switch = 0x20f100254,
		.power_button = 0x20f100184,
		.home_button = 0x20f100180,
		.home_button_at_right = 1
	},
	{
		.name = "iPhone9,1",
		.volume_up = 0x20f10005c,
		.volume_down = 0x20f1002d0,
		.mute_switch = 0x20f100158,
		.power_button = 0x20f1002cc,
		.home_button = NULL,
		.home_button_at_right = 0
	},
	{
		.name = "iPhone9,3",
		.volume_up = 0x20f10005c,
		.volume_down = 0x20f1002d0,
		.mute_switch = 0x20f100158,
		.power_button = 0x20f1002cc,
		.home_button = NULL,
		.home_button_at_right = 0
	},
	{
		.name = "iPad7,4",
		.volume_up = 0x20f1000a0,
		.volume_down = 0x20f1000ac,
		.mute_switch = NULL,
		.power_button = 0x20f100228,
		.home_button = 0x20f10022c,
		.home_button_at_right = 1
	},
	{
		.name = "iPad7,12",
		.volume_up = 0x20f100168,
		.volume_down = 0x20f100164,
		.mute_switch = NULL,
		.power_button = 0x20f1002cc,
		.home_button = 0x20f1002d0,
		.home_button_at_right = 1
	},
	{
		.name = "iPod7,1",
		.volume_up = 0x20e3000b4,
		.volume_down = 0x20e3000b8,
		.mute_switch = NULL,
		.power_button = 0x20e300084,
		.home_button = 0x20e300080,
		.home_button_at_right = 1
	},
	{
		.name = "iPod9,1",
		.volume_up = 0x20f10005c,
		.volume_down = 0x20f1002d0,
		.mute_switch = NULL,
		.power_button = 0x20f1002cc,
		.home_button = 0x20f100158,
		.home_button_at_right = 1
	},
	{
		.name = "iPhone8,4",
		.volume_up = 0x20f100108,
		.volume_down = 0x20f10010c,
		.mute_switch = 0x20f100254,
		.power_button = 0x20f100184,
		.home_button = 0x20f100180,
		.home_button_at_right = 1
	},
	{ .name = NULL }
};

const struct device_model *device = NULL;
uint32_t game_scale;

const uint8_t game[] =
	"\x6E\x05\x65\x00\x6B\x06\x6A\x00\xA3\x0C\xDA\xB1\x7A\x04\x3A\x40"
	"\x12\x08\x7B\x02\x3B\x12\x12\x06\x6C\x20\x6D\x1F\xA3\x10\xDC\xD1"
	"\x22\xF6\x60\x00\x61\x00\xA3\x12\xD0\x11\x70\x08\xA3\x0E\xD0\x11"
	"\x60\x40\xF0\x15\xF0\x07\x30\x00\x12\x34\xC6\x0F\x67\x1E\x68\x01"
	"\x69\xFF\xA3\x0E\xD6\x71\xA3\x10\xDC\xD1\x60\x04\xE0\xA1\x7C\xFE"
	"\x60\x06\xE0\xA1\x7C\x02\x60\x3F\x8C\x02\xDC\xD1\xA3\x0E\xD6\x71"
	"\x86\x84\x87\x94\x60\x3F\x86\x02\x61\x1F\x87\x12\x47\x1F\x12\xAC"
	"\x46\x00\x68\x01\x46\x3F\x68\xFF\x47\x00\x69\x01\xD6\x71\x3F\x01"
	"\x12\xAA\x47\x1F\x12\xAA\x60\x05\x80\x75\x3F\x00\x12\xAA\x60\x01"
	"\xF0\x18\x80\x60\x61\xFC\x80\x12\xA3\x0C\xD0\x71\x60\xFE\x89\x03"
	"\x22\xF6\x75\x01\x22\xF6\x45\x60\x12\xDE\x12\x46\x69\xFF\x80\x60"
	"\x80\xC5\x3F\x01\x12\xCA\x61\x02\x80\x15\x3F\x01\x12\xE0\x80\x15"
	"\x3F\x01\x12\xEE\x80\x15\x3F\x01\x12\xE8\x60\x20\xF0\x18\xA3\x0E"
	"\x7E\xFF\x80\xE0\x80\x04\x61\x00\xD0\x11\x3E\x00\x12\x30\x12\xDE"
	"\x78\xFF\x48\xFE\x68\xFF\x12\xEE\x78\x01\x48\x02\x68\x01\x60\x04"
	"\xF0\x18\x69\xFF\x12\x70\xA3\x14\xF5\x33\xF2\x65\xF1\x29\x63\x37"
	"\x64\x00\xD3\x45\x73\x05\xF2\x29\xD3\x45\x00\xEE\xE0\x00\x80\x00"
	"\xFC\x00\xAA\x00\x00\x00\x00\x00";

#define IS_DOWN(button) ((~*(button)) & 1)

void redraw_screen(chip8_t *self, chip8_event_t event) {
	const uint32_t scale = game_scale;
	const uint8_t flip = device->home_button_at_right;
	const uint32_t yOffset = (gHeight - (CHIP8_SCREEN_WIDTH * scale)) / 2;
	const uint32_t xOffset = (gWidth - (CHIP8_SCREEN_HEIGHT * scale)) / 2;
	for (uint32_t y=event.redraw_event.y; y<event.redraw_event.y + event.redraw_event.height; y++) {
		for (uint32_t x=event.redraw_event.x; x<event.redraw_event.x + event.redraw_event.width; x++) {
			uint32_t rx = x % CHIP8_SCREEN_WIDTH;
			uint32_t ry = y % CHIP8_SCREEN_HEIGHT;
			uint32_t tx = (flip ? rx : (CHIP8_SCREEN_WIDTH - rx - 1));
			uint32_t ty = (flip ? (CHIP8_SCREEN_HEIGHT - ry - 1) : ry);
			for (uint32_t sx = ty * scale; sx < (ty + 1) * scale; sx++) {
				for (uint32_t sy = tx * scale; sy < (tx + 1) * scale; sy++) {
					gFramebuffer[gRowPixels * (sy + yOffset) + (sx + xOffset)] = self->framebuffer[rx][ry] ? 0xFFFFFFFF : 0x0;
				}
			}
		}
	}
}

void play_brix() {
	srand((unsigned int)get_ticks());

	// find the device details
	uint32_t len;
	const char *device_model = (char *)dt_prop(gDeviceTree, "model", &len);
	device = devices;
	while (device->name != NULL) {
		if (strcmp(device->name, device_model) == 0) {
			break;
		}
		device++;
	}
	if (device->name == NULL) {
		printf("This device isn't supported. Use the button helper to add support for this device.\n");
		return;
	}

	// scaling
	game_scale = MIN((gWidth / CHIP8_SCREEN_HEIGHT), (gHeight / CHIP8_SCREEN_WIDTH));

	// buttons
	const volatile uint32_t *pause_button = device->mute_switch ?: device->home_button;
	const volatile uint32_t *right_button = device->home_button_at_right ? device->volume_down : device->volume_up;
	const volatile uint32_t *left_button = device->home_button_at_right ? device->volume_up : device->volume_down;

	do {
		// clear the screen
		for (uint32_t x=0; x<gWidth; x++) {
			for (uint32_t y=0; y<gHeight; y++) {
				gFramebuffer[gRowPixels * y + x] = 0x00000000;
			}
		}

		// start the game
		chip8_t *emulator = chip8_init();
		if (!emulator) return;
		chip8_set_callback(emulator, CHIP8_REDRAW, redraw_screen);
		memcpy(&emulator->memory[0x200], game, sizeof(game));
		const int64_t usec_per_instruction = 1000000 / 500;
		while (!IS_DOWN(device->power_button)) {
			int64_t cycle_start = get_ticks();

			emulator->keyboard_mask = 0;
			emulator->keyboard_mask |= IS_DOWN(left_button) ? (1 << 0xB) : 0x0;
			emulator->keyboard_mask |= IS_DOWN(right_button) ? (1 << 0x9) : 0x0;
			if (!IS_DOWN(pause_button)) {
				chip8_cycle(emulator);
			}

			int64_t cycle_usec = ((int64_t)get_ticks() - cycle_start) / 24;
			int64_t spin_duration = (int64_t)usec_per_instruction - cycle_usec;

			if (spin_duration > 0) {
				spin((uint32_t)spin_duration);
			}
		}
		chip8_free(emulator);
	} while (!IS_DOWN(pause_button));
}

void module_entry() {
	command_register("brix", "starts brix", play_brix);
}

char *module_name = "brix";
struct pongo_exports exported_symbols[] = {
	{.name = 0, .value = 0}
};
