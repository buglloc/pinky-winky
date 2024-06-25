#include "ble.h"

#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/logging/log.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <mbedtls/sha1.h>

#include "btn.h"
#include "batt.h"


LOG_MODULE_REGISTER(pw_ble);

#if CONFIG_ADV_USE_PUB_ADDRESS
#define PW_BLE_EXT_ADV_OPTS BT_LE_ADV_OPT_USE_IDENTITY
#else
#define PW_BLE_EXT_ADV_OPTS 0
#endif

#define IDX_MFG_BATT_LVL  3
#define IDX_MFG_BTN_STATE (IDX_MFG_BATT_LVL + 1)
#define IDX_MFG_TS        (IDX_MFG_BTN_STATE + 1)
#define IDX_MFG_SIGN      (IDX_MFG_TS + 4)
#define MFG_SIGN_LEN      (10)

static uint8_t mfg_data[] = {
    /* company ID must be 0xffff by spec */
    0xff, 0xff,
    /* version */
    0x01,
    /* battery level */
    0x00,
    /* button state */
    0x00,
    /* ts */
    0x00, 0x00, 0x00, 0x00,
    /* truncated to 10 bytes sha1 sign */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const struct bt_data advertising[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR),
    BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, sizeof(CONFIG_BT_DEVICE_NAME) - 1),
    BT_DATA(BT_DATA_MANUFACTURER_DATA, mfg_data, ARRAY_SIZE(mfg_data)),
};

static struct bt_le_ext_adv *adv = NULL;
static uint32_t pw_initial_ts = 0;
static uint32_t pw_last_ts = 0;

static uint8_t mfg_hash_buf[20] = {};
static mbedtls_sha1_context mfg_hash_ctx;

int pw_ble_update_mfg_sign()
{
    int err;

    err = mbedtls_sha1_starts(&mfg_hash_ctx);
    if (err) {
        LOG_ERR("failed to start sha1 hash (err %d)", err);
        return -1;
    }

    err = mbedtls_sha1_update(&mfg_hash_ctx, &mfg_data[0], IDX_MFG_SIGN);
    if (err) {
        LOG_ERR("failed to update hash (err %d)", err);
        return -1;
    }

    err = mbedtls_sha1_update(&mfg_hash_ctx, CONFIG_PW_SIGN_KEY, sizeof(CONFIG_PW_SIGN_KEY) - 1);
    if (err) {
        LOG_ERR("failed to update hash (err %d)", err);
        return -1;
    }

    err = mbedtls_sha1_finish(&mfg_hash_ctx, mfg_hash_buf);
    if (err) {
        LOG_ERR("failed to calculate hash (err %d)", err);
        return -1;
    }

    memcpy(&mfg_data[IDX_MFG_SIGN], mfg_hash_buf, MFG_SIGN_LEN);
    return 0;
}

void pw_ble_update_adv_data()
{
    int err;

    pw_last_ts = pw_initial_ts + k_uptime_seconds();
    sys_put_be32(pw_last_ts, &mfg_data[IDX_MFG_TS]);
    mfg_data[IDX_MFG_BATT_LVL] = pw_batt_percent();
    mfg_data[IDX_MFG_BTN_STATE] = is_pw_btn_pressed() ? 1 : 0;

    err = pw_ble_update_mfg_sign();
    if (err) {
        LOG_ERR("failed to sign mfg data (err %d)", err);
    }

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

    err = bt_rand(&pw_initial_ts, sizeof(uint16_t));
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

    mbedtls_sha1_init(&mfg_hash_ctx);

    err = bt_enable(NULL);
    if (err) {
        LOG_ERR("bluetooth init failed (err %d)", err);
        return -1;
    }

    /* Create a non-connectable advertising set */
    err = bt_le_ext_adv_create(
        BT_LE_ADV_PARAM(
            BT_LE_ADV_OPT_EXT_ADV | PW_BLE_EXT_ADV_OPTS,
            CONFIG_ADV_INTERVAL_MIN, CONFIG_ADV_INTERVAL_MAX,
            NULL
        ),
        NULL,
        &adv
    );
    if (err) {
        LOG_ERR("failed to create advertising set (err %d)", err);
        return -1;
    }

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
