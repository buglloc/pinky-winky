#ifndef H_PW_BLE_
#define H_PW_BLE_

#include <stdbool.h>

#define BLE_ADV_INT_UNIT 0.625

int pw_ble_enable();
int pw_ble_start();
int pw_ble_stop();
int pw_ble_refresh_data();
int pw_ble_refresh_data_now();

#endif
