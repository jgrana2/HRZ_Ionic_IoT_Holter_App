/* Copyright (c) 2017 Horizon Medical SAS, Cartagena - Colombia.
 * All Rights Reserved.
 */

#include "HRZ_ADS1298.h"
#include "HRZ_ecg_service.h"
#include "HRZ_ble.h"

#define NRF_LOG_MODULE_NAME "ADS1298"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_delay.h"

static uint8_t spi_tx_buffer[ADS1298_SPI_BUFFER_SIZE];
static uint8_t spi_rx_buffer[ADS1298_SPI_BUFFER_SIZE];
bool ads1298_data_ready = false;
bool ads1298_data_received = false;
bool ads1298_config_mode_enabled = true;
hrz_ads1298_data_t *ads1298_data;

/**@brief ADS1298 SPI Module init
 */
void hrz_ads1298_spi_init(void){
  nrf_drv_spi_config_t ads1298_spi_config = NRF_DRV_SPI_DEFAULT_CONFIG;
  // ads1298_spi_config.ss_pin   = ADS1298_SPI_SS_PIN;
  ads1298_spi_config.miso_pin = ADS1298_SPI_MISO_PIN;
  ads1298_spi_config.mosi_pin = ADS1298_SPI_MOSI_PIN;
  ads1298_spi_config.sck_pin  = ADS1298_SPI_SCK_PIN;
  ads1298_spi_config.frequency = SPI_FREQUENCY_FREQUENCY_K500;
  ads1298_spi_config.mode = NRF_SPI_MODE_1;
  APP_ERROR_CHECK(nrf_drv_spi_init(&ads1298_spi, &ads1298_spi_config, hrz_ads1298_spi_event_handler, NULL));
  nrf_gpio_cfg_output(ADS1298_SPI_SS_PIN);
  ADS1298_DESELECT();
}

/**
 * @brief ADS1298 SPI event handler.
 * @param event
 */
void hrz_ads1298_spi_event_handler(nrf_drv_spi_evt_t const * p_event, void * p_context)
{
  spi_xfer_done = true;
  //If connected, notifying and not configuring ADS1298 registers
  if (m_ecgs.conn_handle != BLE_CONN_HANDLE_INVALID && ads1298_config_mode_enabled == false) {
    ads1298_data = (hrz_ads1298_data_t *) spi_rx_buffer;
    ads1298_data_received = true;
  }
}

/**
 * @brief Function for configuring ADS1298_INT_PIN for input
 * and GPIOTE to give an interrupt on low to high pin change.
 */
void hrz_ads1298_int_init(void)
{
  ret_code_t err_code;

  nrf_drv_gpiote_in_config_t in_config = GPIOTE_CONFIG_IN_SENSE_HITOLO(true);
  in_config.pull = NRF_GPIO_PIN_PULLUP;

  err_code = nrf_drv_gpiote_in_init(ADS1298_DRDY, &in_config, hrz_ads1298_int_pin_handler);
  APP_ERROR_CHECK(err_code);

}

/**
 * @brief Enable external interrupt
 */
void hrz_enable_ads1298_external_int(void)
{
  nrf_drv_gpiote_in_event_enable(ADS1298_DRDY, true);
}

/**
 * @brief Enable external interrupt
 */
void hrz_disable_ads1298_external_int(void)
{
  nrf_drv_gpiote_in_event_disable(ADS1298_DRDY);
}

/**
 * @brief External DRDY interrupt handler
 * @param pin     DRDY pin input
 * @param action  Action polarity
 */
void hrz_ads1298_int_pin_handler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
    // NRF_LOG_INFO("ADS1298 Interrupt Received\r\n");
    ads1298_data_ready = true;
}

/**
 * @brief ADS1298 initialization routine
 */
void hrz_ads1298_init(void)
{
  ads1298_config_mode_enabled = true;
  hrz_ads1298_spi_init();
  hrz_ads1298_int_init();
  nrf_gpio_cfg_output(ADS1298_PWDN);
  nrf_gpio_cfg_output(ADS1298_RESET);
  nrf_gpio_cfg_output(ADS1298_START);

  //Power up
  nrf_gpio_pin_clear(ADS1298_START);  //Do not start yet
  nrf_gpio_pin_set(ADS1298_PWDN);
  nrf_gpio_pin_set(ADS1298_RESET);

  //Maximum tCLK = 514ns, datasheet p17
  //Wait tPOR = tCLK * 2^18 = 134742016ns ~ 135ms, datasheet p96
  nrf_delay_ms(135);

  //Issue reset pulse 2 * tCLK = 1028ns ~ 2us, datasheet p96
  nrf_gpio_pin_clear(ADS1298_RESET);
  nrf_delay_us(2);
  nrf_gpio_pin_set(ADS1298_RESET);

  //Wait 16 * tCLK = 8224ns ~ 9us, datasheet p96
  nrf_delay_us(9);
  //At this point the device is ready to use
  NRF_LOG_INFO("ADS1298 Initialized\r\n");

  //Device Wakes Up in Read Data Continuous Mode.
  //Send Stop Data Continuous command so that registers can be written
  uint8_t sdatac = ADS1298_SDATACC;
  hrz_ads1298_spi_txrx(&sdatac, 1, 0);
  nrf_delay_ms(200);

  //Read ID
  uint8_t readID[2] = { 0x20, 0x00 };
  hrz_ads1298_spi_txrx(readID, sizeof(readID), 3);

  //Write to registers from CONFIG1 to CH8SET
  uint8_t command[14] =
  { 0x41, 0x0B, 0x46, 0x10, 0xCE, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }; //0x05 = test signal, 0x81 = off, 0x00 = normal electrode input
  hrz_ads1298_spi_txrx(command, sizeof(command), 0);

  //Read registers
  uint8_t readRegs[2] = { 0x21, 0x0B };
  hrz_ads1298_spi_txrx(readRegs, sizeof(readRegs), 14);

  //Enable Read Data Continuous mode
  uint8_t rdatac = ADS1298_RDATACC;
  hrz_ads1298_spi_txrx(&rdatac, 1, 0);

  //Start
  nrf_gpio_pin_set(ADS1298_START);
  ads1298_config_mode_enabled = false;
}

/**
 * @brief ADS1298 SPI transmit receive function
 * @param tx_buf  TX buffer
 * @param tx_len  TX buffer length
 * @param rx_len  RX buffer length
 */
void hrz_ads1298_spi_txrx(uint8_t * tx_buf, uint8_t tx_len, uint8_t rx_len)
{
  // Reset RX buffer and transfer done flag
  memset(spi_rx_buffer, 0, ADS1298_SPI_BUFFER_SIZE);
  spi_xfer_done = false;

  //Start SPI transfer
  ADS1298_SELECT();
  APP_ERROR_CHECK(nrf_drv_spi_transfer(&ads1298_spi, tx_buf, tx_len, spi_rx_buffer, rx_len));

  //Wait for SPI transfer to end
  while (!spi_xfer_done)
  {
    __WFE();
  }
  ADS1298_DESELECT();
}

/**
 * @brief Function for receiving and processing data from ADS1298
 */
void hrz_get_ads1298_data(){
  hrz_ads1298_spi_txrx(spi_tx_buffer, 0, ADS1298_SPI_BUFFER_SIZE);
  while (!ads1298_data_received);
  hrz_send_ecg_channels();
  ads1298_data_ready = false;
}

/**
 * @brief Function for sending all characteristics notifications
 */
void hrz_send_ecg_channels()
{
  ret_code_t err_code;
  static uint8_t hrz_status[HRZ_ECGS_MAX_BUFFER_SIZE];
  static uint8_t hrz_channel1[HRZ_ECGS_MAX_BUFFER_SIZE];
  static uint8_t hrz_channel2[HRZ_ECGS_MAX_BUFFER_SIZE];
  static uint8_t hrz_channel3[HRZ_ECGS_MAX_BUFFER_SIZE];
  static uint8_t hrz_channel4[HRZ_ECGS_MAX_BUFFER_SIZE];
  static uint8_t hrz_channel5[HRZ_ECGS_MAX_BUFFER_SIZE];
  static uint8_t hrz_channel6[HRZ_ECGS_MAX_BUFFER_SIZE];
  static uint8_t hrz_channel7[HRZ_ECGS_MAX_BUFFER_SIZE];
  static uint8_t hrz_channel8[HRZ_ECGS_MAX_BUFFER_SIZE];
  static size_t sample_cnt = 0;

  //Save each channel in its own byte array
  hrz_status[sample_cnt] = ads1298_data->status & 0xFF;
  hrz_status[sample_cnt + 1] = (ads1298_data->status >> 8) & 0xFF;
  hrz_status[sample_cnt + 2] = (ads1298_data->status >> 16) & 0xFF;

  hrz_channel1[sample_cnt] = ads1298_data->channel1 & 0xFF;
  hrz_channel1[sample_cnt + 1] = (ads1298_data->channel1 >> 8) & 0xFF;
  hrz_channel1[sample_cnt + 2] = (ads1298_data->channel1 >> 16) & 0xFF;

  hrz_channel2[sample_cnt] = ads1298_data->channel2 & 0xFF;
  hrz_channel2[sample_cnt + 1] = (ads1298_data->channel2 >> 8) & 0xFF;
  hrz_channel2[sample_cnt + 2] = (ads1298_data->channel2 >> 16) & 0xFF;

  hrz_channel3[sample_cnt] = ads1298_data->channel3 & 0xFF;
  hrz_channel3[sample_cnt + 1] = (ads1298_data->channel3 >> 8) & 0xFF;
  hrz_channel3[sample_cnt + 2] = (ads1298_data->channel3 >> 16) & 0xFF;

  hrz_channel4[sample_cnt] = ads1298_data->channel4 & 0xFF;
  hrz_channel4[sample_cnt + 1] = (ads1298_data->channel4 >> 8) & 0xFF;
  hrz_channel4[sample_cnt + 2] = (ads1298_data->channel4 >> 16) & 0xFF;

  hrz_channel5[sample_cnt] = ads1298_data->channel5 & 0xFF;
  hrz_channel5[sample_cnt + 1] = (ads1298_data->channel5 >> 8) & 0xFF;
  hrz_channel5[sample_cnt + 2] = (ads1298_data->channel5 >> 16) & 0xFF;

  hrz_channel6[sample_cnt] = ads1298_data->channel6 & 0xFF;
  hrz_channel6[sample_cnt + 1] = (ads1298_data->channel6 >> 8) & 0xFF;
  hrz_channel6[sample_cnt + 2] = (ads1298_data->channel6 >> 16) & 0xFF;

  hrz_channel7[sample_cnt] = ads1298_data->channel7 & 0xFF;
  hrz_channel7[sample_cnt + 1] = (ads1298_data->channel7 >> 8) & 0xFF;
  hrz_channel7[sample_cnt + 2] = (ads1298_data->channel7 >> 16) & 0xFF;

  hrz_channel8[sample_cnt] = ads1298_data->channel8 & 0xFF;
  hrz_channel8[sample_cnt + 1] = (ads1298_data->channel8 >> 8) & 0xFF;
  hrz_channel8[sample_cnt + 2] = (ads1298_data->channel8 >> 16) & 0xFF;

  sample_cnt += HRZ_CHANNEL_LEN;

  //If channel buffer is filled, send notifications
  if (sample_cnt >= HRZ_ECGS_MAX_BUFFER_SIZE) {

    err_code = hrz_ecg_send(&m_ecgs, m_ecgs.ecg_status_handles, hrz_status, HRZ_ECGS_MAX_BUFFER_SIZE);
    if (err_code == NRF_ERROR_RESOURCES) {
      //Wait until buffer is free and send again
      buffer_is_free = false;
      while (buffer_is_free == false);
      err_code = hrz_ecg_send(&m_ecgs, m_ecgs.ecg_status_handles, hrz_status, HRZ_ECGS_MAX_BUFFER_SIZE);
      NRF_LOG_INFO("BFS\r\n");
    }else{
      SEND_ERROR_CHECK(err_code);
    }

    err_code = hrz_ecg_send(&m_ecgs, m_ecgs.ecg_channel_1_handles, hrz_channel1, HRZ_ECGS_MAX_BUFFER_SIZE);
    if (err_code == NRF_ERROR_RESOURCES) {
      buffer_is_free = false;
      while (buffer_is_free == false);
      err_code = hrz_ecg_send(&m_ecgs, m_ecgs.ecg_channel_1_handles, hrz_channel1, HRZ_ECGS_MAX_BUFFER_SIZE);
      NRF_LOG_INFO("BF1\r\n");
    }else{
      SEND_ERROR_CHECK(err_code);
    }

    err_code = hrz_ecg_send(&m_ecgs, m_ecgs.ecg_channel_2_handles, hrz_channel2, HRZ_ECGS_MAX_BUFFER_SIZE);
    if (err_code == NRF_ERROR_RESOURCES) {
      buffer_is_free = false;
      while (buffer_is_free == false);
      err_code = hrz_ecg_send(&m_ecgs, m_ecgs.ecg_channel_2_handles, hrz_channel2, HRZ_ECGS_MAX_BUFFER_SIZE);
      NRF_LOG_INFO("BF2\r\n");
    }else{
      SEND_ERROR_CHECK(err_code);
    }

    err_code = hrz_ecg_send(&m_ecgs, m_ecgs.ecg_channel_3_handles, hrz_channel3, HRZ_ECGS_MAX_BUFFER_SIZE);
    if (err_code == NRF_ERROR_RESOURCES) {
      buffer_is_free = false;
      while (buffer_is_free == false);
      err_code = hrz_ecg_send(&m_ecgs, m_ecgs.ecg_channel_3_handles, hrz_channel3, HRZ_ECGS_MAX_BUFFER_SIZE);
      NRF_LOG_INFO("BF3\r\n");
    }else{
      SEND_ERROR_CHECK(err_code);
    }

    err_code = hrz_ecg_send(&m_ecgs, m_ecgs.ecg_channel_4_handles, hrz_channel4, HRZ_ECGS_MAX_BUFFER_SIZE);
    if (err_code == NRF_ERROR_RESOURCES) {
      buffer_is_free = false;
      while (buffer_is_free == false);
      err_code = hrz_ecg_send(&m_ecgs, m_ecgs.ecg_channel_4_handles, hrz_channel4, HRZ_ECGS_MAX_BUFFER_SIZE);
      NRF_LOG_INFO("BF4\r\n");
    }else{
      SEND_ERROR_CHECK(err_code);
    }

    err_code = hrz_ecg_send(&m_ecgs, m_ecgs.ecg_channel_5_handles, hrz_channel5, HRZ_ECGS_MAX_BUFFER_SIZE);
    if (err_code == NRF_ERROR_RESOURCES) {
      buffer_is_free = false;
      while (buffer_is_free == false);
      err_code = hrz_ecg_send(&m_ecgs, m_ecgs.ecg_channel_5_handles, hrz_channel5, HRZ_ECGS_MAX_BUFFER_SIZE);
      NRF_LOG_INFO("BF5\r\n");
    }else{
      SEND_ERROR_CHECK(err_code);
    }

    err_code = hrz_ecg_send(&m_ecgs, m_ecgs.ecg_channel_6_handles, hrz_channel6, HRZ_ECGS_MAX_BUFFER_SIZE);
    if (err_code == NRF_ERROR_RESOURCES) {
      buffer_is_free = false;
      while (buffer_is_free == false);
      err_code = hrz_ecg_send(&m_ecgs, m_ecgs.ecg_channel_6_handles, hrz_channel6, HRZ_ECGS_MAX_BUFFER_SIZE);
      NRF_LOG_INFO("BF6\r\n");
    }else{
      SEND_ERROR_CHECK(err_code);
    }

    err_code = hrz_ecg_send(&m_ecgs, m_ecgs.ecg_channel_7_handles, hrz_channel7, HRZ_ECGS_MAX_BUFFER_SIZE);
    if (err_code == NRF_ERROR_RESOURCES) {
      buffer_is_free = false;
      while (buffer_is_free == false);
      err_code = hrz_ecg_send(&m_ecgs, m_ecgs.ecg_channel_7_handles, hrz_channel7, HRZ_ECGS_MAX_BUFFER_SIZE);
      NRF_LOG_INFO("BF7\r\n");
    }else{
      SEND_ERROR_CHECK(err_code);
    }

    err_code = hrz_ecg_send(&m_ecgs, m_ecgs.ecg_channel_8_handles, hrz_channel8, HRZ_ECGS_MAX_BUFFER_SIZE);
    if (err_code == NRF_ERROR_RESOURCES) {
      buffer_is_free = false;
      while (buffer_is_free == false);
      err_code = hrz_ecg_send(&m_ecgs, m_ecgs.ecg_channel_8_handles, hrz_channel8, HRZ_ECGS_MAX_BUFFER_SIZE);
      NRF_LOG_INFO("BF8\r\n");
    }else{
      SEND_ERROR_CHECK(err_code);
    }

    //Reset channel arrays just to make sure
    memset(hrz_channel1, 0, HRZ_ECGS_MAX_BUFFER_SIZE);
    memset(hrz_channel2, 0, HRZ_ECGS_MAX_BUFFER_SIZE);
    memset(hrz_channel3, 0, HRZ_ECGS_MAX_BUFFER_SIZE);
    memset(hrz_channel4, 0, HRZ_ECGS_MAX_BUFFER_SIZE);
    memset(hrz_channel5, 0, HRZ_ECGS_MAX_BUFFER_SIZE);
    memset(hrz_channel6, 0, HRZ_ECGS_MAX_BUFFER_SIZE);
    memset(hrz_channel7, 0, HRZ_ECGS_MAX_BUFFER_SIZE);
    memset(hrz_channel8, 0, HRZ_ECGS_MAX_BUFFER_SIZE);

    //Fill channel arrays again
    sample_cnt = 0;
  }
}
