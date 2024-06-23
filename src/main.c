#include <zephyr/logging/log.h>

#include "led.h"
#include "btn.h"
#include "ble.h"


LOG_MODULE_REGISTER(pw_main);

int main(void)
{
	int err;
	LOG_INF("Initializing");

	LOG_INF("Setup LEDs");
	err = pw_led_enable();
	if (err) {
		LOG_ERR("couldn't initialize LED subsystem (err %d)", err);
		return 0;
	}

	LOG_INF("Setup Buttons");
	err = pw_btn_enable();
	if (err) {
		LOG_ERR("couldn't initialize buttons subsystem (err %d)", err);
		return 0;
	}

	LOG_INF("Setup BLE");
	err = pw_ble_enable();
	if (err) {
		LOG_ERR("couldn't initialize BLE subsystem (err %d)", err);
		return 0;
	}

	LOG_INF("PinkyWinky was started");
	return 0;
}
