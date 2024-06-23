#include "led.h"

#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>


LOG_MODULE_REGISTER(pw_led);

#define LED_NODE DT_ALIAS(led0)
#if !DT_NODE_HAS_STATUS(LED_NODE, okay)
#error "Unsupported board: led0 devicetree alias is not defined"
#endif
static struct gpio_dt_spec pw_led = GPIO_DT_SPEC_GET(LED_NODE, gpios);


int pw_led_enable()
{
	int err;

	if (!device_is_ready(pw_led.port)) {
		LOG_ERR("LED device %s is not ready", pw_led.port->name);
		return -1;
	}

	err = gpio_pin_configure_dt(&pw_led, GPIO_OUTPUT_INACTIVE);
	if (err) {
		LOG_ERR("couldn't configure LED device %s (err %d)", pw_led.port->name, err);
		return -1;
	}

	LOG_INF("LED initialized at %s pin %d", pw_led.port->name, pw_led.pin);
	return 0;
}

void pw_led_off()
{
    gpio_pin_set_dt(&pw_led, 0);
}

void pw_led_on()
{
    gpio_pin_set_dt(&pw_led, 1);
}
