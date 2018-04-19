/* Copyright (c) 2017 Horizon Medical SAS, Cartagena - Colombia.
 * All Rights Reserved.
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "HRZ_ble.h"
#include "HRZ_ADS1298.h"

#ifndef FIR_FILTER_ENABLED
#define FIR_FILTER_ENABLED 0
#endif

#define NRF_LOG_MODULE_NAME "MAIN"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"

/**@brief Function for application main entry.
 */
int main(void)
{
    bool erase_bonds;

    //Presentation
    log_init();
    NRF_LOG_RAW_INFO("\r\n");
    NRF_LOG_RAW_INFO("  HORIZON MEDICAL\r\n");
    NRF_LOG_RAW_INFO("IoT Holter BLE v1.0\r\n");
    NRF_LOG_RAW_INFO("===================\r\n");
    NRF_LOG_RAW_INFO("\r\n");

    // Initialize.
    timers_init();
    buttons_leds_init(&erase_bonds);
    ble_stack_init();
    gap_params_init();
    gatt_init();
    advertising_init();
    services_init();
    hrz_ads1298_init();
    conn_params_init();
    peer_manager_init();

    //Start execution
    application_timers_start();
    advertising_start(erase_bonds);

    // Enter main loop.
    for (;;)
    {
        if (NRF_LOG_PROCESS() == false)
        {
            if (ads1298_data_ready) {
              hrz_get_ads1298_data();
            }
            power_manage();
        }
    }
}

// void app_error_fault_handler(uint32_t id, uint32_t pc, uint32_t info)
// {
//     NRF_LOG_ERROR("Error id: %d\r\n", info);
//     NRF_LOG_FINAL_FLUSH();
//     app_error_save_and_stop(id, pc, info);
// }
