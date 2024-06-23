#include "ble.h"

#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/logging/log.h>
#include <zephyr/bluetooth/bluetooth.h>

#include "btn.h"


LOG_MODULE_REGISTER(pw_ble);

#if CONFIG_ADV_USE_PUB_ADDRESS
#define PW_BLE_EXT_ADV_OPTS BT_LE_ADV_OPT_USE_IDENTITY
#else
#define PW_BLE_EXT_ADV_OPTS 0
#endif

static uint8_t mfg_data[] = {
	/* company ID must be 0xffff by spec */
	0xff, 0xff,
	/* version */
	0x01,
	/* button state */
	0x01,
	/* ts */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	/* md5 sign */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const struct bt_data advertising[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR),
	BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, sizeof(CONFIG_BT_DEVICE_NAME) - 1),
	BT_DATA(BT_DATA_MANUFACTURER_DATA, mfg_data, ARRAY_SIZE(mfg_data)),
};

static struct bt_le_ext_adv *adv = NULL;
static uint64_t initial_ts = 0;

void pw_ble_update_adv_data()
{
	int err;

	mfg_data[0x03] = pw_btn_is_pressed() ? 1 : 0;
	sys_put_be64(initial_ts, &mfg_data[0x04]);

    err = bt_le_ext_adv_set_data(adv, advertising, ARRAY_SIZE(advertising), NULL, 0);
    if (err) {
        LOG_ERR("failed to update adv data (err %d)", err);
    }
}

void pw_ble_refresh_adv_data(struct k_work *work)
{
	pw_ble_update_adv_data();
}

K_WORK_DEFINE(refresh_adv_data_work, pw_ble_refresh_adv_data);


int pw_ble_init_adv_data()
{
	int err;

	err = bt_rand(&initial_ts, sizeof(uint32_t));
	if (err) {
		LOG_ERR("couldn't generate initial ts (err %d)", err);
		return -1;
	}

	pw_ble_update_adv_data();
	return 0;
}


int pw_ble_enable()
{
	int err;

	/* Initialize the Bluetooth Subsystem */
	err = bt_enable(NULL);
	if (err) {
		LOG_ERR("bluetooth init failed (err %d)", err);
		return -1;
	}

	/* Create a non-connectable advertising set */
	err = bt_le_ext_adv_create(
		BT_LE_ADV_PARAM(
			BT_LE_ADV_OPT_EXT_ADV | PW_BLE_EXT_ADV_OPTS,
			CONFIG_ADV_INTERVAL_MIN_MS, CONFIG_ADV_INTERVAL_MAX_MS,
			NULL
		),
		NULL,
		&adv
	);
	if (err) {
		LOG_ERR("failed to create advertising set (err %d)", err);
		return -1;
	}

	/* Set advertising data */
	err = pw_ble_init_adv_data();
	if (err) {
		LOG_ERR("advertising init failed (err %d)", err);
		return -1;
	}

    LOG_INF("start extended advertising...");
    err = bt_le_ext_adv_start(adv, BT_LE_EXT_ADV_START_DEFAULT);
    if (err) {
        LOG_ERR("failed to start extended advertising (err %d)", err);
        return -1;
    }

    LOG_INF("BLE initialized");
    return 0;
}

int pw_ble_refresh_data()
{
    return k_work_submit(&refresh_adv_data_work);
}
