#include "batt.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/adc.h>

LOG_MODULE_REGISTER(pw_batt);

#if CONFIG_PW_BATT && DT_NODE_EXISTS(DT_PATH(zephyr_user)) && \
    DT_NODE_HAS_PROP(DT_PATH(zephyr_user), io_channels) && \
    DT_NODE_HAS_PROP(DT_PATH(zephyr_user), io_channel_names)

static const struct adc_dt_spec adc_channel = ADC_DT_SPEC_GET_BY_NAME(DT_PATH(zephyr_user), batt);
static uint8_t pw_batt_cur_percent = 0;

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
        return -2;
    }

    LOG_INF("Battery initialized at %s", adc_channel.dev->name);
    return pw_batt_refresh_data();
}

uint8_t pw_batt_percent()
{
    return pw_batt_cur_percent;
}

int pw_batt_refresh_data()
{
    int err;
    uint16_t sample_buff = 0;
    struct adc_sequence sequence = {
        .buffer = &sample_buff,
        .buffer_size = sizeof(sample_buff),
    };

    err = adc_sequence_init_dt(&adc_channel, &sequence);
    if (err) {
        LOG_ERR("could not initialize dt (err %d)", err);
        return -1;
    }

    err = adc_read_dt(&adc_channel, &sequence);
    if (err) {
        LOG_ERR("could not read dt (err %d)", err);
        return -2;
    }

    int32_t val_mv = (int32_t)sample_buff;
    err = adc_raw_to_millivolts_dt(&adc_channel, &val_mv);
    if (err) {
        LOG_ERR("could not convert dt voltage (err %d)", err);
        return -3;
    }

    if (val_mv <= CONFIG_PW_BATT_MIN_VOLTAGE) {
        pw_batt_cur_percent = 0;
        return 0;
    }

    if (val_mv >= CONFIG_PW_BATT_MAX_VOLTAGE) {
        pw_batt_cur_percent = 100;
        return 0;
    }

    val_mv -= CONFIG_PW_BATT_MIN_VOLTAGE;
    val_mv *= 100;
    val_mv /= CONFIG_PW_BATT_MAX_VOLTAGE - CONFIG_PW_BATT_MIN_VOLTAGE;
    pw_batt_cur_percent = (uint8_t) val_mv;
    return 0;
}

#else

int pw_batt_enable()
{
    LOG_WRN("Battery is disabled or not available");
    return 0;
}

uint8_t pw_batt_percent()
{
    return 0;
}

int pw_batt_refresh_data()
{
    return 0;
}

#endif
