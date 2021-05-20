// This code was only tested on 2 devices: A1778 and A1709.
// Other devices may use different address ranges or bits
// for the buttons and if they do, this code will not work.

// "Holding the mute switch" means muting
// "Releasing the mute switch" means unmuting

#include <stdio.h>
#include "pongo.h"

#define IS_DOWN(button) ((~*(button)) & 1)
#define NOT_FOUND (uint32_t *)0x0
#define TOO_MANY_MATCHES (uint32_t *)0x1
#define SKIPPED (uint32_t *)0x2

const volatile uint32_t * const gpio_base = 0x20f100000;

// fflush(stdout) wasn't working so this function forces a flush
void flush_spin(uint32_t usec) {
	for (uint32_t i=0; i<usec/1000; i++) {
		printf(".\b");
		spin(1000);
	}
	printf(".\n");
}

void find_button(const char *name, uint8_t *possible_addresses) {
	printf("Hold \"%s\"", name);
	flush_spin(3000000);
	for (int i=0; i<0x400; i++) {
		if (!IS_DOWN(gpio_base + i)) {
			possible_addresses[i] = 0;
		}
	}
	printf("Release \"%s\" and keep it released", name);
	flush_spin(3000000);
	for (int i=0; i<0x400; i++) {
		if (IS_DOWN(gpio_base + i)) {
			possible_addresses[i] = 0;
		}
	}
}

void find_buttons() {
	uint32_t len;
	const char *device_model = (char *)dt_prop(gDeviceTree, "model", &len);
	if (!device_model) {
		device_model = "(unknown)";
	}
	printf("\nDevice: %s\n\n", device_model);

	static uint8_t possible_addresses[0x400];

	const char *names[] = { "volume_down", "volume_up", "mute_switch", "power_button", "home_button", 0 };
	const char **name = names;
	while (*name != NULL) {
		printf("Get ready");
		flush_spin(2500000);
		const volatile uint32_t *address = NOT_FOUND;
		int count = 0;
		while ((address == NOT_FOUND) || (address == TOO_MANY_MATCHES)) {
			for (int i=0; i<0x100; i++) {
				((uint32_t *)possible_addresses)[i] = 0x01010101;
			}
			find_button(*name, possible_addresses);
			for (int i=0; i<0x400; i++) {
				if (possible_addresses[i]) {
					if (address != NOT_FOUND) {
						address = TOO_MANY_MATCHES;
						break;
					}
					address = gpio_base + i;
				}
			}
			if (address == NOT_FOUND) {
				printf("Couldn't find any matching addresses. Skipping.\n\n");
				address = SKIPPED;
			}
			else if (address != TOO_MANY_MATCHES) {
				printf("Address: %p\n\n", address);
			}
			else if (++count >= 3) {
				printf("Couldn't find address. Possible matches:\n");
				for (int i=0; i<0x400; i++) {
					if (!possible_addresses[i]) continue;
					printf("- %p\n", gpio_base + i);
				}
				printf("\n");
				address = SKIPPED;
			}
		}
		name++;
	}
}

void module_entry() {
	command_register("button_helper", "command for finding button addresses", find_buttons);
}

char *module_name = "button_finder";
struct pongo_exports exported_symbols[] = {
	{.name = 0, .value = 0}
};