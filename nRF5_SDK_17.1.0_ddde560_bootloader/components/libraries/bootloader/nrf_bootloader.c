/**
 * Copyright (c) 2016 - 2021, Nordic Semiconductor ASA
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 *
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 *
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
#include "nrf_bootloader.h"

#include "compiler_abstraction.h"
#include "nrf.h"
#include "boards.h"
#include "sdk_config.h"
#include "nrf_power.h"
#include "nrf_delay.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_dfu.h"
#include "nrf_error.h"
#include "nrf_dfu_settings.h"
#include "nrf_dfu_utils.h"
#include "nrf_bootloader_wdt.h"
#include "nrf_bootloader_info.h"
#include "nrf_bootloader_app_start.h"
#include "nrf_bootloader_fw_activation.h"
#include "nrf_bootloader_dfu_timers.h"
#include "app_scheduler.h"
#include "nrf_dfu_validation.h"
#include "nrf_drv_uart.h"

#include "slip.h"

#define MAIN_FILE_NAME "FOTA.txt"
//#define FILE_NAME "app.bin"
#define FRAME_SIZE 128


#define MAX_COMM_RETRIES   3
extern nrf_drv_uart_t m_uart;
const uint8_t read_file_cmd[] = "AT+QFREAD=%d,%s\r";
const uint8_t close_file_cmd[] = "AT+QFCLOSE=%d\r";
const uint8_t connect_str[] = "\r\nCONNECT \r\n";
const uint8_t ok_str[] = "\r\nOK\r\n";
uint8_t data_tx[128];
uint32_t start_of_data;
uint8_t len_data_str[5]; //up to 9999
uint32_t current_frame_size;
uint8_t state = INIT;
bool is_response_ready = true;
uint8_t dfu_mode = DFU_UART_MODE;

uint8_t main_file_handle = 0;
uint8_t dat_file_handle = 0;
uint8_t fw_file_handle = 0;
uint8_t main_file_size = 0;
uint32_t dat_file_size;

uint32_t fw_file_size;
uint8_t error_code = 0;
uint8_t new_fw_file_name[32];
uint8_t old_fw_file_name[32];
uint8_t current_fw_file_name[32];
bool is_fw_file_open = false;
bool is_dat_file_open = false;
bool is_main_file_open = false;
uint8_t main_file_data[128];
uint8_t upgrade_retries;
const uint8_t fota_file_template[] = "Old=%s\r\n" \
                                      "Current=%s\r\n" \
                                      "Upgrade Retries=%d\r\n" \
                                      "Status=%s";

uint8_t num_retries = 0;


uint8_t upgrade_status = UPGRADE_STATUS_PENDING;
uint8_t comm_retries = 0;
int32_t string_size;
int32_t size_len_str;
        

static nrf_dfu_observer_t m_user_observer; //<! Observer callback set by the user.
static volatile bool m_flash_write_done;

#define SCHED_QUEUE_SIZE      32          /**< Maximum number of events in the scheduler queue. */
#define SCHED_EVENT_DATA_SIZE NRF_DFU_SCHED_EVENT_DATA_SIZE /**< Maximum app_scheduler event size. */

#if !(defined(NRF_BL_DFU_ENTER_METHOD_BUTTON)    && \
      defined(NRF_BL_DFU_ENTER_METHOD_PINRESET)  && \
      defined(NRF_BL_DFU_ENTER_METHOD_GPREGRET)  && \
      defined(NRF_BL_DFU_ENTER_METHOD_BUTTONLESS)&& \
      defined(NRF_BL_RESET_DELAY_MS)             && \
      defined(NRF_BL_DEBUG_PORT_DISABLE))
    #error Configuration file is missing flags. Update sdk_config.h.
#endif

STATIC_ASSERT((NRF_BL_DFU_INACTIVITY_TIMEOUT_MS >= 100) || (NRF_BL_DFU_INACTIVITY_TIMEOUT_MS == 0),
             "NRF_BL_DFU_INACTIVITY_TIMEOUT_MS must be 100 ms or more, or 0 to indicate that it is disabled.");

#if defined(NRF_LOG_BACKEND_FLASH_START_PAGE)
STATIC_ASSERT(NRF_LOG_BACKEND_FLASH_START_PAGE != 0,
    "If nrf_log flash backend is used it cannot use space after code because it would collide with settings page.");
#endif

/**@brief Weak implemenation of nrf_dfu_init
 *
 * @note   This function will be overridden if nrf_dfu.c is
 *         compiled and linked with the project
 */
 #if (__LINT__ != 1)
__WEAK uint32_t nrf_dfu_init(nrf_dfu_observer_t observer)
{
    NRF_LOG_DEBUG("in weak nrf_dfu_init");
    return NRF_SUCCESS;
}
#endif


/**@brief Weak implementation of nrf_dfu_init
 *
 * @note    This function must be overridden in application if
 *          user-specific initialization is needed.
 */
__WEAK uint32_t nrf_dfu_init_user(void)
{
    NRF_LOG_DEBUG("in weak nrf_dfu_init_user");
    return NRF_SUCCESS;
}


static void flash_write_callback(void * p_context)
{
    UNUSED_PARAMETER(p_context);
    m_flash_write_done = true;
}


static void do_reset(void * p_context)
{
    UNUSED_PARAMETER(p_context);

    NRF_LOG_FINAL_FLUSH();

    nrf_delay_ms(NRF_BL_RESET_DELAY_MS);

    NVIC_SystemReset();
}


static void bootloader_reset(bool do_backup)
{
    NRF_LOG_DEBUG("Resetting bootloader.");

    if (do_backup)
    {
        m_flash_write_done = false;
        nrf_dfu_settings_backup(do_reset);
    }
    else
    {
        do_reset(NULL);
    }
}


static void inactivity_timeout(void)
{
    NRF_LOG_INFO("Inactivity timeout.");

    if (dfu_mode == DFU_UART_MODE)
    {
        nrf_bootloader_dfu_inactivity_timer_restart(
                            NRF_BOOTLOADER_MS_TO_TICKS(NRF_BL_DFU_INACTIVITY_TIMEOUT_MS),
                            inactivity_timeout);

        comm_retries++;

        if (comm_retries > MAX_COMM_RETRIES)
        {
            //TODO: Launch BLE DFU because something is happening with BG77 comm
            s_dfu_settings.dfu_mode = 0xc2;
            s_dfu_settings.enter_buttonless_dfu = 1;
            nrf_dfu_settings_write_and_backup(NULL);
            bootloader_reset(false);
        }
        else
        {
            nrf_bootloader_dfu_inactivity_timer_restart(
                            NRF_BOOTLOADER_MS_TO_TICKS(NRF_BL_DFU_INACTIVITY_TIMEOUT_MS),
                            inactivity_timeout);
            state = RESET_MODEM;
            is_response_ready = true;
        }
    }
    else
    {
        bootloader_reset(true);
    }
}


/**@brief Function for handling DFU events.
 */
static void dfu_observer(nrf_dfu_evt_type_t evt_type)
{
    switch (evt_type)
    {
        case NRF_DFU_EVT_DFU_STARTED:
        case NRF_DFU_EVT_OBJECT_RECEIVED:
            nrf_bootloader_dfu_inactivity_timer_restart(
                        NRF_BOOTLOADER_MS_TO_TICKS(NRF_BL_DFU_INACTIVITY_TIMEOUT_MS),
                        inactivity_timeout);
            break;
        case NRF_DFU_EVT_DFU_COMPLETED:
        case NRF_DFU_EVT_DFU_ABORTED:
            bootloader_reset(true);
            break;
        case NRF_DFU_EVT_TRANSPORT_DEACTIVATED:
            // Reset the internal state of the DFU settings to the last stored state.
            nrf_dfu_settings_reinit();
            break;
        default:
            break;
    }

    if (m_user_observer)
    {
        m_user_observer(evt_type);
    }
}


/**@brief Function for initializing the event scheduler.
 */
static void scheduler_init(void)
{
    APP_SCHED_INIT(SCHED_EVENT_DATA_SIZE, SCHED_QUEUE_SIZE);
}


/**@brief Suspend the CPU until an interrupt occurs.
 */
static void wait_for_event(void)
{
#if defined(BLE_STACK_SUPPORT_REQD) || defined(ANT_STACK_SUPPORT_REQD)
    (void)sd_app_evt_wait();
#else
    // Wait for an event.
    __WFE();
    // Clear the internal event register.
    __SEV();
    __WFE();
#endif
}


/**@brief Continually sleep and process tasks whenever woken.
 */
static void loop_forever(void)
{
    while (true)
    {
        //feed the watchdog if enabled.
        nrf_bootloader_wdt_feed();

        app_sched_execute();

        if (is_response_ready)
        {
          switch(state)
          {
              case RESET_MODEM:
              {
                  nrf_bootloader_dfu_inactivity_timer_restart(
                            NRF_BOOTLOADER_MS_TO_TICKS(NRF_BL_DFU_INACTIVITY_TIMEOUT_MS),
                            inactivity_timeout);
                  string_size = snprintf(data_tx, sizeof(data_tx), "AT+CFUN=1,1\r");
                  is_response_ready = false;
                  nrf_drv_uart_tx(&m_uart, data_tx, string_size);
              }break;
              case INIT:
              {
                  nrf_bootloader_dfu_inactivity_timer_restart(
                            NRF_BOOTLOADER_MS_TO_TICKS(NRF_BL_DFU_INACTIVITY_TIMEOUT_MS),
                            inactivity_timeout);
                  self_init();
                  nrf_delay_ms(100);
                  string_size = snprintf(data_tx, sizeof(data_tx), "AT\r");
                  is_response_ready = false;
                  ret_code_t ret_code = nrf_drv_uart_tx(&m_uart, data_tx, string_size);
              }break;
              case GET_MAIN_FILE_SIZE:
              {
                string_size = snprintf(data_tx, sizeof(data_tx), "AT+QFLST=\"%s\"\r", MAIN_FILE_NAME);
                is_response_ready = false;
                nrf_drv_uart_tx(&m_uart, data_tx, string_size);
              }break;
              case OPEN_MAIN_FILE:
              {
                  string_size = snprintf(data_tx, sizeof(data_tx), "AT+QFOPEN=\"%s\",0\r", MAIN_FILE_NAME);
                  is_response_ready = false;
                  nrf_drv_uart_tx(&m_uart, data_tx, string_size);
              }break;
              case GET_FW_INFO_FROM_MAIN_FILE:
              {
                   is_main_file_open = true;
                   size_len_str = snprintf(len_data_str, sizeof(len_data_str), "%d", main_file_size);

                   // calculate the start of binary data (after "\r\nCONNECT {len_data_str}\r\n")
                   start_of_data = strlen(connect_str) + size_len_str;
            
                   // Put together read command plus length of data to read ("AT+QFREAD=2,{len_data_str}\r")
                   string_size = snprintf(data_tx, sizeof(data_tx), read_file_cmd, main_file_handle, len_data_str);

                   set_frame_size(HEADER_READ_SIZE + main_file_size + size_len_str);

                   is_response_ready = false;
                   nrf_drv_uart_tx(&m_uart, data_tx, string_size);
              }break;
              case GET_DAT_FILE_SIZE:
              {
                  //TODO: remove this is stament, it is only for debug
                  if (s_dfu_settings.progress.command_offset != 0)
                  {
                      s_dfu_settings.progress.command_offset = 0;
                  }
                  if (upgrade_status == UPGRADE_STATUS_PENDING)
                  {
                      if (new_fw_file_name[0] != NULL)
                      {
                          string_size = snprintf(data_tx, sizeof(data_tx), "AT+QFLST=\"%s.dat\"\r", new_fw_file_name);
                          is_response_ready = false;
                          nrf_drv_uart_tx(&m_uart, data_tx, string_size);
                      }
                      else
                      {
                          state = UPGRADE_FAILED;
                      }
                  }
                  else
                  {
                      state = WRITE_UPGRADE_STATUS_SET_POINTER;
                  }
              }break;
              case GET_DATA_FROM_DAT_FILE:
              {
                  is_dat_file_open = true;
                  current_frame_size = dat_file_size -s_dfu_settings.progress.command_offset;
                  if (current_frame_size > FRAME_SIZE)
                  {
                      current_frame_size = FRAME_SIZE;
                  }

                  if(current_frame_size == 0)
                  {
                       state = EXCECUTE_DAT_FILE;
                  }
                  else
                  {

                       size_len_str = snprintf(len_data_str, sizeof(len_data_str), "%d", current_frame_size);

                       // calculate the start of binary data (after "\r\nCONNECT {len_data_str}\r\n")
                       start_of_data = strlen(connect_str) + size_len_str;
            
                       // Put together read command plus length of data to read ("AT+QFREAD=2,{len_data_str}\r")
                       string_size = snprintf(data_tx, sizeof(data_tx), read_file_cmd, dat_file_handle, len_data_str);
                       
                       set_frame_size(HEADER_READ_SIZE + current_frame_size + size_len_str);

                       is_response_ready = false;
                       nrf_drv_uart_tx(&m_uart, data_tx, string_size);
                   }
              }break;
              case EXCECUTE_DAT_FILE:
              {
                  uint32_t addr;
                  uint32_t length;
                  //TODO: add ret value and check it
                  ret_code_t ret_val = nrf_dfu_validation_init_cmd_execute(&addr, &length);
                  if (ret_val == NRF_DFU_RES_CODE_SUCCESS)
                  {
                      state = GET_FILE_SIZE;
                  }
                  else
                  {
                      state = UPGRADE_FAILED;
                  }
                  
              }break;
              case OPEN_DAT_FILE:
              {
                   string_size = snprintf(data_tx, sizeof(data_tx), "AT+QFOPEN=\"%s.dat\",2\r",new_fw_file_name);

                   is_response_ready = false;
                   nrf_drv_uart_tx(&m_uart, data_tx, string_size);
              }break;
              case GET_FILE_SIZE:
              {
                string_size = snprintf(data_tx, sizeof(data_tx), "AT+QFLST=\"%s.bin\"\r", new_fw_file_name);

                is_response_ready = false;
                nrf_drv_uart_tx(&m_uart, data_tx, string_size);
              }break;
              case OPEN_FW_FILE:
              {
                nrf_bootloader_dfu_inactivity_timer_restart(
                        NRF_BOOTLOADER_MS_TO_TICKS(NRF_BL_DFU_INACTIVITY_TIMEOUT_MS),
                        inactivity_timeout);
                if (set_app_start_address_and_erase_memory() == NRF_SUCCESS)
                {
                    force_init_valid();
                    string_size = snprintf(data_tx, sizeof(data_tx), "AT+QFOPEN=\"%s.bin\",2\r",new_fw_file_name);

                    is_response_ready = false;
                    nrf_drv_uart_tx(&m_uart, data_tx, string_size);
                }
                else
                {
                    //TODO: close files to avoid error at opening the file after reset
                    NRF_LOG_DEBUG("ERROR while erasing flash!");
                    dfu_observer(NRF_DFU_EVT_DFU_ABORTED);
                }
              }break;

              case WRITE:
              {
                nrf_bootloader_dfu_inactivity_timer_restart(
                        NRF_BOOTLOADER_MS_TO_TICKS(NRF_BL_DFU_INACTIVITY_TIMEOUT_MS),
                        inactivity_timeout);

                is_fw_file_open = true;
                current_frame_size = fw_file_size - s_dfu_settings.write_offset;
                if (current_frame_size > FRAME_SIZE)
                {
                    current_frame_size = FRAME_SIZE;
                }

                if(current_frame_size == 0)
                {
                    upgrade_status = UPGRADE_STATUS_SUCCESS;
                    state = CLOSE_MAIN_FILE;
                }
                else
                {
                    // convert current_frame_size to string and get the length of the generated string
                    size_len_str = snprintf(len_data_str, sizeof(len_data_str), "%d", current_frame_size);

                    // calculate the start of binary data (after "\r\nCONNECT {len_data_str}\r\n")
                    start_of_data = strlen(connect_str) + size_len_str;
            
                    // Put together read command plus length of data to read ("AT+QFREAD=2,{len_data_str}\r")
                    string_size = snprintf(data_tx, sizeof(data_tx), read_file_cmd, fw_file_handle, len_data_str);

                    set_frame_size(HEADER_READ_SIZE + current_frame_size + size_len_str);

                    is_response_ready = false;
                    nrf_drv_uart_tx(&m_uart, data_tx, string_size);
                }
              }break;
              case CLOSE_MAIN_FILE:
              {
                  nrf_bootloader_dfu_inactivity_timer_restart(
                            NRF_BOOTLOADER_MS_TO_TICKS(NRF_BL_DFU_INACTIVITY_TIMEOUT_MS),
                            inactivity_timeout);
                  if (is_main_file_open)
                  {
                      string_size = snprintf(data_tx, sizeof(data_tx), close_file_cmd, main_file_handle);
                      is_response_ready = false;
                      nrf_drv_uart_tx(&m_uart, data_tx, string_size);
                  }
                  else
                  {
                      state = CLOSE_DAT_FILE;
                  }
              }break;
              case CLOSE_DAT_FILE:
              {
                  nrf_bootloader_dfu_inactivity_timer_restart(
                            NRF_BOOTLOADER_MS_TO_TICKS(NRF_BL_DFU_INACTIVITY_TIMEOUT_MS),
                            inactivity_timeout);
                  if (is_dat_file_open)
                  {
                      string_size = snprintf(data_tx, sizeof(data_tx), close_file_cmd, dat_file_handle);
                      is_response_ready = false;
                      nrf_drv_uart_tx(&m_uart, data_tx, string_size);
                  }
                  else
                  {
                      state = CLOSE_FW_FILE;
                  }
              }break;
              case CLOSE_FW_FILE:
              {
                  nrf_bootloader_dfu_inactivity_timer_restart(
                        NRF_BOOTLOADER_MS_TO_TICKS(NRF_BL_DFU_INACTIVITY_TIMEOUT_MS),
                        inactivity_timeout);
                  if (is_fw_file_open)
                  {
                      string_size = snprintf(data_tx, sizeof(data_tx), close_file_cmd, fw_file_handle);
                      is_response_ready = false;
                      nrf_drv_uart_tx(&m_uart, data_tx, string_size);
                  }
                  else
                  {
                      state = START_APP;
                  }
              }break;
              case UPGRADE_FAILED:
              {
                  num_retries++;

                  if (num_retries >= upgrade_retries)
                  {
                      NRF_LOG_WARNING("Upgrade retries limit reached\n")
                      upgrade_status = UPGRADE_STATUS_ERROR;
                      //Prepare FOTA.txt file and write failed on status
                      if (is_main_file_open)
                      {
                          state = WRITE_UPGRADE_STATUS_SET_POINTER;
                      }
                      else
                      {
                          state = GET_MAIN_FILE_SIZE;
                      }
                  }
                  else
                  {
                      state = RESET_MODEM;
                  }               
                  
              }break;
              case WRITE_UPGRADE_STATUS_SET_POINTER:
              {
                  if (is_main_file_open)
                  {
                      string_size = snprintf(data_tx, sizeof(data_tx), "AT+QFSEEK=%d,%d\r", main_file_handle, 0);
                      is_response_ready = false;
                      nrf_drv_uart_tx(&m_uart, data_tx, string_size);
                  }
                  else
                  {
                      //TODO: Main file not open, probably is no present
                      // Launch BLE DFU
                      //TODO: this must be added when BLE DFU and UART DFU are combined.
                      // For now we will reset the modem
                      state = RESET_MODEM;
                  }
              }break;
              case WRITE_UPGRADE_STATUS_TRUNCATE_FILE:
              {
                  string_size = snprintf(data_tx, sizeof(data_tx), "AT+QFTUCAT=%d\r", main_file_handle);
                  is_response_ready = false;
                  nrf_drv_uart_tx(&m_uart, data_tx, string_size);
              }break;
              case WRITE_UPGRADE_STATUS_SEND_CMD:
              {
                  if (upgrade_status == UPGRADE_STATUS_COMPLETED)
                  {
                      string_size = snprintf(data_tx, sizeof(data_tx), fota_file_template, current_fw_file_name, new_fw_file_name, 0, "Success");
                  }
                  else
                  {
                      string_size = snprintf(data_tx, sizeof(data_tx), fota_file_template, old_fw_file_name, current_fw_file_name, 0, "Failed");                  
                  }
                  string_size = snprintf(data_tx, sizeof(data_tx), "AT+QFWRITE=%d,%d,%d\r", main_file_handle, string_size, 2);
                  is_response_ready = false;
                  nrf_drv_uart_tx(&m_uart, data_tx, string_size);

              }break;
              case WRITE_UPGRADE_STATUS_SEND_DATA:
              {
                  if (upgrade_status == UPGRADE_STATUS_COMPLETED)
                  {
                      string_size = snprintf(data_tx, sizeof(data_tx), fota_file_template, current_fw_file_name, new_fw_file_name, 0, "Success");
                  }
                  else
                  {
                      string_size = snprintf(data_tx, sizeof(data_tx), fota_file_template, old_fw_file_name, current_fw_file_name, 0, "Failed");                  
                  }
                  is_response_ready = false;
                  nrf_drv_uart_tx(&m_uart, data_tx, string_size);
              }break;
              case START_APP:
              {
                  nrf_bootloader_dfu_inactivity_timer_restart(
                            NRF_BOOTLOADER_MS_TO_TICKS(NRF_BL_DFU_INACTIVITY_TIMEOUT_MS),
                            inactivity_timeout);
                  //TODO: something went wrong, launch BLE DFU or main app if any
                  if (upgrade_status == UPGRADE_STATUS_ERROR)
                  {
                      s_dfu_settings.dfu_mode = 0xc2;
                      s_dfu_settings.enter_buttonless_dfu = 1;
                      nrf_dfu_settings_write_and_backup(do_reset);
                      //bootloader_reset(false);
                  }
                  else if (upgrade_status == UPGRADE_STATUS_SUCCESS)
                  {           
                      uint8_t* dummy;
                      dfu_init_command_t p_init;
                      p_init.type = DFU_FW_TYPE_APPLICATION;
                      if (on_data_obj_execute_request((nrf_dfu_request_t*)&dummy, (nrf_dfu_response_t*)&dummy))
                      {
                          // we should not get here, something went wrong
                          state = UPGRADE_FAILED;
                          upgrade_status = UPGRADE_STATUS_PENDING;
                          nrf_bootloader_dfu_inactivity_timer_restart(
                                    NRF_BOOTLOADER_MS_TO_TICKS(NRF_BL_DFU_INACTIVITY_TIMEOUT_MS),
                                    inactivity_timeout);
                      }
                      
                  }
                  else // upgrade_status == UPGRADE_STATUS_COMPLETED
                  {
                      bootloader_reset(false);
                  }
              }break;
              default:
              break;
            }
        }

        if (!NRF_LOG_PROCESS())
        {
            wait_for_event();
        }
    }
}

#if NRF_BL_DFU_ENTER_METHOD_BUTTON
#ifndef BUTTON_PULL
    #error NRF_BL_DFU_ENTER_METHOD_BUTTON is enabled but not buttons seem to be available on the board.
#endif
/**@brief Function for initializing button used to enter DFU mode.
 */
static void dfu_enter_button_init(void)
{
    nrf_gpio_cfg_sense_input(NRF_BL_DFU_ENTER_METHOD_BUTTON_PIN,
                             BUTTON_PULL,
                             NRF_GPIO_PIN_SENSE_LOW);
}
#endif


static bool crc_on_valid_app_required(void)
{
    bool ret = true;
    if (NRF_BL_APP_CRC_CHECK_SKIPPED_ON_SYSTEMOFF_RESET &&
        (nrf_power_resetreas_get() & NRF_POWER_RESETREAS_OFF_MASK))
    {
        nrf_power_resetreas_clear(NRF_POWER_RESETREAS_OFF_MASK);
        ret = false;
    }
    else if (NRF_BL_APP_CRC_CHECK_SKIPPED_ON_GPREGRET2 &&
            ((nrf_power_gpregret2_get() & BOOTLOADER_DFU_SKIP_CRC_MASK) == BOOTLOADER_DFU_SKIP_CRC))
    {
        nrf_power_gpregret2_set(nrf_power_gpregret2_get() & ~BOOTLOADER_DFU_SKIP_CRC);
        ret = false;
    }
    else
    {
    }

    return ret;
}



static bool boot_validate(boot_validation_t const * p_validation, uint32_t data_addr, uint32_t data_len, bool do_crc)
{
    if (!do_crc && (p_validation->type == VALIDATE_CRC))
    {
        return true;
    }
    return nrf_dfu_validation_boot_validate(p_validation, data_addr, data_len);
}


/** @brief Function for checking if the main application is valid.
 *
 * @details     This function checks if there is a valid application
 *              located at Bank 0.
 *
 * @param[in]   do_crc Perform CRC check on application. Only CRC checks
                       can be skipped. For other boot validation types,
                       this parameter is ignored.
 *
 * @retval  true  If a valid application has been detected.
 * @retval  false If there is no valid application.
 */
static bool app_is_valid(bool do_crc)
{
    if (s_dfu_settings.bank_0.bank_code != NRF_DFU_BANK_VALID_APP)
    {
        NRF_LOG_INFO("Boot validation failed. No valid app to boot.");
        return false;
    }
    else if (NRF_BL_APP_SIGNATURE_CHECK_REQUIRED &&
        (s_dfu_settings.boot_validation_app.type != VALIDATE_ECDSA_P256_SHA256))
    {
        NRF_LOG_WARNING("Boot validation failed. The boot validation of the app must be a signature check.");
        return false;
    }
    else if (SD_PRESENT && !boot_validate(&s_dfu_settings.boot_validation_softdevice, MBR_SIZE, s_dfu_settings.sd_size, do_crc))
    {
        NRF_LOG_WARNING("Boot validation failed. SoftDevice is present but invalid.");
        return false;
    }
    else if (!boot_validate(&s_dfu_settings.boot_validation_app, nrf_dfu_bank0_start_addr(), s_dfu_settings.bank_0.image_size, do_crc))
    {
        NRF_LOG_WARNING("Boot validation failed. App is invalid.");
        return false;
    }
    // The bootloader itself is not checked, since a self-check of this kind gives little to no benefit
    // compared to the cost incurred on each bootup.

    // if the app was upgraded via BLE return to default (UART)
    if (dfu_mode == DFU_BLE_MODE)
    {
        s_dfu_settings.dfu_mode = 0x2c;
        // Clear DFU flag in flash settings.
        s_dfu_settings.enter_buttonless_dfu = 0;
        nrf_dfu_settings_write_and_backup(NULL);
    }
    NRF_LOG_DEBUG("App is valid");
    return true;
}



/**@brief Function for clearing all DFU enter flags that
 *        preserve state during reset.
 *
 * @details This is used to make sure that each of these flags
 *          is checked only once after reset.
 */
static void dfu_enter_flags_clear(void)
{
    if (NRF_BL_DFU_ENTER_METHOD_PINRESET &&
       (NRF_POWER->RESETREAS & POWER_RESETREAS_RESETPIN_Msk))
    {
        // Clear RESETPIN flag.
        NRF_POWER->RESETREAS |= POWER_RESETREAS_RESETPIN_Msk;
    }

    if (NRF_BL_DFU_ENTER_METHOD_GPREGRET &&
       ((nrf_power_gpregret_get() & BOOTLOADER_DFU_START_MASK) == BOOTLOADER_DFU_START))
    {
        // Clear DFU mark in GPREGRET register.
        nrf_power_gpregret_set(nrf_power_gpregret_get() & ~BOOTLOADER_DFU_START);
    }

    if (NRF_BL_DFU_ENTER_METHOD_BUTTONLESS &&
       (s_dfu_settings.enter_buttonless_dfu == 1))
    {
        // Clear DFU flag in flash settings.
        s_dfu_settings.enter_buttonless_dfu = 0;
        APP_ERROR_CHECK(nrf_dfu_settings_write(NULL));
    }
}


/**@brief Function for checking whether to enter DFU mode or not.
 */
static bool dfu_enter_check(void)
{
    if (!app_is_valid(crc_on_valid_app_required()))
    {
        NRF_LOG_DEBUG("DFU mode because app is not valid.");
        return true;
    }

    if (NRF_BL_DFU_ENTER_METHOD_BUTTON &&
       (nrf_gpio_pin_read(NRF_BL_DFU_ENTER_METHOD_BUTTON_PIN) == 0))
    {
        NRF_LOG_DEBUG("DFU mode requested via button.");
        return true;
    }

    if (NRF_BL_DFU_ENTER_METHOD_PINRESET &&
       (NRF_POWER->RESETREAS & POWER_RESETREAS_RESETPIN_Msk))
    {
        NRF_LOG_DEBUG("DFU mode requested via pin-reset.");
        return true;
    }

    if (NRF_BL_DFU_ENTER_METHOD_GPREGRET &&
       ((nrf_power_gpregret_get() & BOOTLOADER_DFU_START_MASK) == BOOTLOADER_DFU_START))
    {
        NRF_LOG_DEBUG("DFU mode requested via GPREGRET.");
        return true;
    }

    if (NRF_BL_DFU_ENTER_METHOD_BUTTONLESS &&
       (s_dfu_settings.enter_buttonless_dfu == 1))
    {
        NRF_LOG_DEBUG("DFU mode requested via bootloader settings.");
        return true;
    }

    return false;
}


#if NRF_BL_DFU_ALLOW_UPDATE_FROM_APP
static void postvalidate(void)
{
    NRF_LOG_INFO("Postvalidating update after reset.");
    nrf_dfu_validation_init();

    if (nrf_dfu_validation_init_cmd_present())
    {
        uint32_t firmware_start_addr;
        uint32_t firmware_size;

        // Execute a previously received init packed. Subsequent executes will have no effect.
        if (nrf_dfu_validation_init_cmd_execute(&firmware_start_addr, &firmware_size) == NRF_DFU_RES_CODE_SUCCESS)
        {
            if (nrf_dfu_validation_prevalidate() == NRF_DFU_RES_CODE_SUCCESS)
            {
                if (nrf_dfu_validation_activation_prepare(firmware_start_addr, firmware_size) == NRF_DFU_RES_CODE_SUCCESS)
                {
                    NRF_LOG_INFO("Postvalidation successful.");
                }
            }
        }
    }

    s_dfu_settings.bank_current = NRF_DFU_CURRENT_BANK_0;
    UNUSED_RETURN_VALUE(nrf_dfu_settings_write_and_backup(flash_write_callback));
}
#endif


ret_code_t nrf_bootloader_init(nrf_dfu_observer_t observer)
{
    NRF_LOG_DEBUG("In nrf_bootloader_init");

    ret_code_t                            ret_val;
    nrf_bootloader_fw_activation_result_t activation_result;
    uint32_t                              initial_timeout;
    bool                                  dfu_enter = false;

    m_user_observer = observer;

    if (NRF_BL_DEBUG_PORT_DISABLE)
    {
        nrf_bootloader_debug_port_disable();
    }

#if NRF_BL_DFU_ENTER_METHOD_BUTTON
    dfu_enter_button_init();
#endif

    ret_val = nrf_dfu_settings_init(false);
    if (ret_val != NRF_SUCCESS)
    {
        return NRF_ERROR_INTERNAL;
    }

    if (s_dfu_settings.dfu_mode == 0xc2)
    {
        is_response_ready = false;
        dfu_mode = DFU_BLE_MODE;  
    }

    #if NRF_BL_DFU_ALLOW_UPDATE_FROM_APP
    // Postvalidate if DFU has signaled that update is ready.
    if (s_dfu_settings.bank_current == NRF_DFU_CURRENT_BANK_1)
    {
        postvalidate();
    }
    #endif

    // Check if an update needs to be activated and activate it.
    activation_result = nrf_bootloader_fw_activate();

    switch (activation_result)
    {
        case ACTIVATION_NONE:
            initial_timeout = NRF_BOOTLOADER_MS_TO_TICKS(NRF_BL_DFU_INACTIVITY_TIMEOUT_MS);
            dfu_enter       = dfu_enter_check();
            break;

        case ACTIVATION_SUCCESS_EXPECT_ADDITIONAL_UPDATE:
            initial_timeout = NRF_BOOTLOADER_MS_TO_TICKS(NRF_BL_DFU_CONTINUATION_TIMEOUT_MS);
            dfu_enter       = true;
            break;

        case ACTIVATION_SUCCESS:
            // activation of the new FW is success, the bootloader needs to update main file
            if (dfu_mode == DFU_UART_MODE)
            {
                upgrade_status = UPGRADE_STATUS_COMPLETED;
                state = INIT;
                dfu_enter       = true;
                break;
            }
            else
            {
                bootloader_reset(true);
                NRF_LOG_ERROR("Unreachable");
                return NRF_ERROR_INTERNAL; // Should not reach this.
            }
            
        case ACTIVATION_ERROR:
        default:
            return NRF_ERROR_INTERNAL;
    }

    if (dfu_mode == DFU_BLE_MODE)
    {
        // Original BL tiemout is 120 s (5*24 = 120)
        initial_timeout *= 24; 
    }

    if (dfu_enter)
    {
        nrf_bootloader_wdt_init();
        scheduler_init();
        dfu_enter_flags_clear();

        // Call user-defined init function if implemented
        ret_val = nrf_dfu_init_user();
        if (ret_val != NRF_SUCCESS)
        {
            return NRF_ERROR_INTERNAL;
        }

        nrf_bootloader_dfu_inactivity_timer_restart(initial_timeout, inactivity_timeout);

        
        ret_val = nrf_dfu_init(dfu_observer);
        if (ret_val != NRF_SUCCESS)
        {
            return NRF_ERROR_INTERNAL;
        }

        NRF_LOG_DEBUG("Enter main loop");
        loop_forever(); // This function will never return.
        NRF_LOG_ERROR("Unreachable");
    }
    else
    {
        // Erase additional data like peer data or advertisement name
        ret_val = nrf_dfu_settings_additional_erase();
        if (ret_val != NRF_SUCCESS)
        {
            return NRF_ERROR_INTERNAL;
        }

        m_flash_write_done = false;
        nrf_dfu_settings_backup(flash_write_callback);
        ASSERT(m_flash_write_done);

        nrf_bootloader_app_start();
        NRF_LOG_ERROR("Unreachable");
    }

    // Should not be reached.
    return NRF_ERROR_INTERNAL;
}
