#include "batt.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/adc.h>

LOG_MODULE_REGISTER(pw_batt);

#if !DT_NODE_EXISTS(DT_PATH(zephyr_user)) || \
    !DT_NODE_HAS_PROP(DT_PATH(zephyr_user), io_channels) || \
    !DT_NODE_HAS_PROP(DT_PATH(zephyr_user), io_channels_names)

static const struct adc_dt_spec adc_channel = ADC_DT_SPEC_GET_BY_NAME(DT_PATH(zephyr_user), batt);
static uint16_t pw_adc_buf;
static struct adc_sequence pw_adc_sequence = {
    .buffer = &pw_adc_buf,
    /* buffer size in bytes, not number of samples */
    .buffer_size = sizeof(pw_adc_buf),
};

int pw_batt_enable()
{
    int err;

    if (!adc_is_ready_dt(&adc_channel)) {
        LOG_ERR("ADC controller device %s is not ready", adc_channel.dev->name);
        return -1;
    }

    err = adc_channel_setup_dt(&adc_channel);
    if (err < 0) {
        LOG_ERR("could not setup ADC channel (err %d)", err);
        return -1;
    }

    err = adc_sequence_init_dt(&adc_channel, &pw_adc_sequence);
    if (err) {
        LOG_ERR("could not initialize dt (err %d)", err);
        return -1;
    }

    LOG_INF("Battery initialized at %s", adc_channel.dev->name);
    return 0;
}

uint8_t pw_batt_percent()
{
    int err;

    err = adc_read_dt(&adc_channel, &pw_adc_sequence);
    if (err) {
        LOG_ERR("could not read dt (err %d)", err);
        return 1;
    }

    int32_t val_mv = (int32_t)pw_adc_buf;
    err = adc_raw_to_millivolts_dt(&adc_channel, &val_mv);
    if (err) {
        LOG_ERR("could not convert dt voltage (err %d)", err);
        return 2;
    }

    uint8_t percent = val_mv * 100 / CONFIG_PW_BATT_VOLTAGE;
    if (percent > 100) {
        return 100;
    }

    return percent;
}

#else

int pw_batt_enable()
{
    return 0;
}

uint8_t pw_batt_percent()
{
    return 0;
}

#endif