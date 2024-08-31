#include "btn.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/atomic.h>

#include "led.h"
#include "ble.h"


LOG_MODULE_REGISTER(pw_btn);

#define BUTTON_DEBOUNCE_DELAY_MS (100)
#define BUTTON_ON_STATE_TTL (CONFIG_PW_BUTTON_INTERVAL * BLE_ADV_INT_UNIT)

#define SW_NODE DT_ALIAS(button_sw)
#if !DT_NODE_HAS_STATUS(SW_NODE, okay)
#error "Unsupported board: sw0 devicetree alias is not defined"
#endif

static const struct gpio_dt_spec pw_btn = GPIO_DT_SPEC_GET(SW_NODE, gpios);
static struct gpio_callback pw_btn_cb_data;

static atomic_val_t pw_btn_pressed;
static uint32_t pw_btn_last_time = 0;

void pw_btn_cb(const struct device *dev, struct gpio_callback *cb, uint32_t pins);

void pw_clear_btn_pressed()
{
    atomic_clear_bit(&pw_btn_pressed, 0);
}

void pw_set_btn_pressed()
{
    atomic_set_bit(&pw_btn_pressed, 0);
}

bool pw_is_btn_pressed() {
    return atomic_test_bit(&pw_btn_pressed, 0);
}

int pw_btn_enable()
{
    int err;

    pw_clear_btn_pressed();
    if (!device_is_ready(pw_btn.port)) {
        LOG_ERR("button device %s is not ready", pw_btn.port->name);
        return -1;
    }

    err = gpio_pin_configure_dt(&pw_btn, GPIO_INPUT);
    if (err) {
        LOG_ERR("failed to configure %s pin %d (err %d)", pw_btn.port->name, pw_btn.pin, err);
        return -1;
    }

    err = gpio_pin_interrupt_configure_dt(&pw_btn, GPIO_INT_EDGE_TO_ACTIVE);
    if (err) {
        LOG_ERR("failed to configure interrupt on %s pin %d (err %d)", pw_btn.port->name, pw_btn.pin, err);
        return -1;
    }

    pw_btn_last_time = k_uptime_get_32();
    gpio_init_callback(&pw_btn_cb_data, pw_btn_cb, BIT(pw_btn.pin));
    gpio_add_callback(pw_btn.port, &pw_btn_cb_data);
    LOG_INF("button initialized at %s pin %d", pw_btn.port->name, pw_btn.pin);
    return 0;
}

void pw_btn_release_cb(struct k_timer *dummy)
{
    pw_clear_btn_pressed();
    pw_led_off();

    pw_ble_refresh_data_now();
}

K_TIMER_DEFINE(pw_btn_release_timer, pw_btn_release_cb, NULL);

void pw_btn_cb(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
    uint32_t time = k_uptime_get_32();

    if (time < pw_btn_last_time + BUTTON_DEBOUNCE_DELAY_MS) {
        return;
    }

    LOG_INF("button pressed at %" PRIu64, k_uptime_get());
    pw_btn_last_time = time;
    pw_set_btn_pressed();
    pw_led_on();
    pw_ble_refresh_data_now();
    k_timer_start(&pw_btn_release_timer, K_MSEC(BUTTON_ON_STATE_TTL), K_NO_WAIT);
}
