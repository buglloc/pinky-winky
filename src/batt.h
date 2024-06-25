#ifndef H_PW_BATT_
#define H_PW_BATT_

#include <stdint.h>


int pw_batt_enable();
int pw_batt_refresh_data();
uint8_t pw_batt_percent();

#endif
