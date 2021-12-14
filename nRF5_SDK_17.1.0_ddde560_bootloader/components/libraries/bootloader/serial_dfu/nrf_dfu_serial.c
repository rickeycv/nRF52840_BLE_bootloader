/**
 * Copyright (c) 2017 - 2021, Nordic Semiconductor ASA
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

#include "nrf_dfu_serial.h"
#include "nrf_dfu_req_handler.h"
#include "nrf_dfu_handling_error.h"
#include "nrf_dfu_settings.h"

#include "slip.h"
#include <stdlib.h>

#define NRF_LOG_MODULE_NAME nrf_dfu_serial
#include "nrf_log.h"
NRF_LOG_MODULE_REGISTER();

#define NRF_SERIAL_OPCODE_SIZE (sizeof(uint8_t))

extern uint8_t state;
extern bool is_response_ready;
extern uint8_t main_file_handle;
extern uint16_t main_file_size;
extern uint8_t fw_file_handle;
extern uint32_t fw_file_size;
extern uint8_t dat_file_handle;
extern uint32_t dat_file_size;
extern uint8_t error_code;
extern uint8_t new_fw_file_name[32];
extern uint8_t old_fw_file_name[32];
extern uint8_t current_fw_file_name[32];
extern uint8_t upgrade_retries;
extern bool is_fw_file_open;
extern bool is_dat_file_open;
extern bool is_main_file_open;

uint8_t* p_str;
uint8_t* tokens[4];
uint32_t index_tk;
bool is_error;

uint8_t upgrade_restries_str[3]; // up to 99 

#if defined(NRF_DFU_PROTOCOL_REDUCED) && NRF_DFU_PROTOCOL_REDUCED
    #error Serial DFU (UART and USB) cannot function with the reduced protocol set.
#endif

static uint32_t response_ext_err_payload_add(uint8_t * p_buffer, uint32_t buf_offset)
{
    p_buffer[buf_offset] = ext_error_get();
    (void) ext_error_set(NRF_DFU_EXT_ERROR_NO_ERROR);

    return 1;
}


static void response_send(nrf_dfu_serial_t         * p_transport,
                          nrf_dfu_response_t const * p_response)
{
    uint8_t   index            = 0;
    uint8_t * p_serialized_rsp = p_transport->p_rsp_buf;

    NRF_LOG_DEBUG("Sending Response: [0x%01x, 0x%01x]", p_response->request, p_response->result);

    p_serialized_rsp[index++] = NRF_DFU_OP_RESPONSE;
    p_serialized_rsp[index++] = p_response->request;
    p_serialized_rsp[index++] = (uint8_t)(p_response->result);

    if (p_response->result == NRF_DFU_RES_CODE_SUCCESS)
    {
        switch (p_response->request)
        {
            case NRF_DFU_OP_PROTOCOL_VERSION:
            {
                p_serialized_rsp[index] = p_response->protocol.version;
                index += sizeof(uint8_t);
            } break;

            case NRF_DFU_OP_HARDWARE_VERSION:
            {
                index += uint32_encode(p_response->hardware.part,                 &p_serialized_rsp[index]);
                index += uint32_encode(p_response->hardware.variant,              &p_serialized_rsp[index]);
                index += uint32_encode(p_response->hardware.memory.rom_size,      &p_serialized_rsp[index]);
                index += uint32_encode(p_response->hardware.memory.ram_size,      &p_serialized_rsp[index]);
                index += uint32_encode(p_response->hardware.memory.rom_page_size, &p_serialized_rsp[index]);
            } break;

            case NRF_DFU_OP_FIRMWARE_VERSION:
            {
                p_serialized_rsp[index++] = p_response->firmware.type;
                index += uint32_encode(p_response->firmware.version, &p_serialized_rsp[index]);
                index += uint32_encode(p_response->firmware.addr,    &p_serialized_rsp[index]);
                index += uint32_encode(p_response->firmware.len,     &p_serialized_rsp[index]);
            } break;

            case NRF_DFU_OP_CRC_GET:
                index += uint32_encode(p_response->crc.offset, &p_serialized_rsp[index]);
                index += uint32_encode(p_response->crc.crc,    &p_serialized_rsp[index]);
                break;

            case NRF_DFU_OP_OBJECT_SELECT:
                index += uint32_encode(p_response->select.max_size, &p_serialized_rsp[index]);
                index += uint32_encode(p_response->select.offset,   &p_serialized_rsp[index]);
                index += uint32_encode(p_response->select.crc,      &p_serialized_rsp[index]);
                break;

            case NRF_DFU_OP_MTU_GET:
                index += uint16_encode(p_response->mtu.size, &p_serialized_rsp[index]);
                break;

            case NRF_DFU_OP_PING:
                p_serialized_rsp[index] = p_response->ping.id;
                index += sizeof(uint8_t);
                break;

            default:
                // no implementation
                break;
        }
    }
    else if (p_response->result == NRF_DFU_RES_CODE_EXT_ERROR)
    {
        index += response_ext_err_payload_add(p_serialized_rsp, index);
    }

    if (index > NRF_SERIAL_MAX_RESPONSE_SIZE)
    {
        NRF_LOG_ERROR("Message is larger than expected.");
    }

    // Send response.
    if (p_transport->rsp_func((uint8_t const *)(p_serialized_rsp), index) != NRF_SUCCESS)
    {
        NRF_LOG_ERROR("Failed to send data over serial interface!");
    }
}


void dfu_req_handler_rsp_clbk(nrf_dfu_response_t * p_res, void * p_context)
{
    nrf_dfu_serial_t * p_transport = (nrf_dfu_serial_t *)(p_context);

    if (p_res->result != NRF_DFU_RES_CODE_SUCCESS)
    {
        NRF_LOG_WARNING("DFU request completed with result: 0x%x", p_res->result);
    }

    switch (p_res->request)
    {
        default:
            /* Reply normally.
             * Make sure to reply to NRF_DFU_OP_OBJECT_CREATE when running DFU over serial,
             * otherwise the transfer might run very slow, without an apparent reason.
             */
            break;

        case NRF_DFU_OP_OBJECT_WRITE:
        {
            p_transport->pkt_notif_target_count--;

            if (   (p_transport->pkt_notif_target       == 0)
                || (p_transport->pkt_notif_target_count != 0))
            {
                is_response_ready = true;
                /* Do not reply to _OBJECT_WRITE messages. */
                return;
            }

            /* Reply with a CRC message and reset the packet counter. */
            p_transport->pkt_notif_target_count = p_transport->pkt_notif_target;

            p_res->request    = NRF_DFU_OP_CRC_GET;
            p_res->crc.offset = p_res->write.offset;
            p_res->crc.crc    = p_res->write.crc;
        } break;
    }

    // comment this line because we not need to reply to the modem 
    //response_send(p_transport, p_res);
}

void get_str_value(const uint8_t *str_in, uint8_t *str_out, uint8_t *token, uint8_t *delim)
{
    uint8_t *p_str;
    uint8_t *p_str_end;

    p_str = strstr(str_in, token);

    if (p_str != NULL)
    {
        p_str += strlen(token); 

        p_str_end = strstr(p_str,delim);

        if (p_str_end != NULL)
        {
            memcpy(str_out, p_str, p_str_end - p_str);
        }
        else
        {
            str_out[0] = 0;
        }
    }
    else
    {
        str_out[0] = 0;
    }
}


void nrf_dfu_serial_on_packet_received(nrf_dfu_serial_t       * p_transport,
                                       uint8_t          const * p_data,
                                       uint32_t                 length)
{
    uint8_t  const * p_payload   = &p_data[NRF_SERIAL_OPCODE_SIZE];
    uint16_t const   payload_len = (length - NRF_SERIAL_OPCODE_SIZE);

    
    nrf_dfu_request_t request =
    {
        .request           = (nrf_dfu_op_t)(p_data[0]),
        .callback.response = dfu_req_handler_rsp_clbk,
        .p_context         = p_transport
    };

    bool buf_free = true;

    switch (request.request)
    {
        case NRF_DFU_OP_FIRMWARE_VERSION:
        {
            request.firmware.image_number = p_payload[0];
        } break;

        case NRF_DFU_OP_RECEIPT_NOTIF_SET:
        {
            NRF_LOG_DEBUG("Set receipt notif target: %d", p_transport->pkt_notif_target);

            p_transport->pkt_notif_target       = uint16_decode(&p_payload[0]);
            p_transport->pkt_notif_target_count = p_transport->pkt_notif_target;
        } break;

        case NRF_DFU_OP_OBJECT_SELECT:
        {
            request.select.object_type = p_payload[0];
        } break;

        case NRF_DFU_OP_OBJECT_CREATE:
        {
            // Reset the packet receipt notification on create object
            p_transport->pkt_notif_target_count = p_transport->pkt_notif_target;

            request.create.object_type = p_payload[0];
            request.create.object_size = uint32_decode(&p_payload[1]);

            if (request.create.object_type == NRF_DFU_OBJ_TYPE_COMMAND)
            {
                /* Activity on the current transport. Close all except the current one. */
                (void) nrf_dfu_transports_close(p_transport->p_low_level_transport);
            }
        } break;

        case NRF_DFU_OP_OBJECT_WRITE:
        {
            // Buffer will be freed asynchronously
            buf_free = false;

            request.write.p_data   = p_payload;
            request.write.len      = payload_len;
            request.callback.write = p_transport->payload_free_func;
        } break;

        case NRF_DFU_OP_MTU_GET:
        {
            NRF_LOG_DEBUG("Received serial mtu");
            request.mtu.size = p_transport->mtu;
        } break;

        case NRF_DFU_OP_PING:
        {
            NRF_LOG_DEBUG("Received ping %d", p_payload[0]);
            request.ping.id = p_payload[0];
        } break;
        case RESET_MODEM:
        {
            state = INIT;
            is_response_ready = true;
        }break;
        case INIT:
        {
            p_str = strstr(p_payload,"OK");
            if(p_str != NULL)
            {
                state = GET_MAIN_FILE_SIZE;
            }
            else
            {
                UPGRADE_FAILED;
            }
            
            is_response_ready = true;
        }break;
        case GET_MAIN_FILE_SIZE:
        {
            is_error = false;
            index_tk = 0;
            remove_char((uint8_t *)p_payload, '\r');
            remove_char((uint8_t *)p_payload, '\n');
            tokens[index_tk++] = strtok((uint8_t *)p_payload, " ,");
            do
            {
                tokens[index_tk] = strtok(NULL, " ,");
            }while(tokens[index_tk++] != NULL);
            p_str = strstr(tokens[2],"OK");
            if(p_str != NULL)
            {
                *p_str = '\0';

                if(strcmp("+QFLST:", tokens[0]) == 0)
                {
                
                    for (uint32_t i = 0; i < strlen(tokens[2]); i++)
                    {
                        if(tokens[2][i] < '0' || tokens[2][i] > '9')
                        {
                            is_error= true;
                            break;
                        }
                    }
                    if (!is_error)
                    {
                        main_file_size = atoi(tokens[2]);
                    }
                    else
                    {
                        is_error= true;
                    }
                }
                else
                {
                    is_error= true;
                }
            }
            else
            {
                is_error= true;
            }

            if (!is_error)
            {
                state = OPEN_MAIN_FILE;
            }
            else
            {
                state = UPGRADE_FAILED;
            }
            is_response_ready = true;
            

        }break;
        case OPEN_MAIN_FILE:
        {
            is_error = false;
            index_tk = 0;
            remove_char((uint8_t *)p_payload, '\r');
            remove_char((uint8_t *)p_payload, '\n');
            tokens[index_tk++] = strtok((uint8_t *)p_payload, " ,");
            do
            {
                tokens[index_tk] = strtok(NULL, " ,");
            }while(tokens[index_tk++] != NULL);
            p_str = strstr(tokens[1],"OK");
            if(p_str != NULL)
            {
                *p_str = '\0';
                if(strcmp("+QFOPEN:", tokens[0]) == 0)
                {
                
                    for (uint32_t i = 0; i < strlen(tokens[1]); i++)
                    {
                        if(tokens[1][i] < '0' || tokens[1][i] > '9')
                        {
                            is_error= true;
                            break;
                        }
                    }
                    if (!is_error)
                    {
                        main_file_handle = atoi(tokens[1]);
                        state = GET_FW_INFO_FROM_MAIN_FILE;
                    }
                    else
                    {
                        state = UPGRADE_FAILED;
                    }
                }
                else
                {
                    state = UPGRADE_FAILED;
                }
            }
            else
            {
                state = UPGRADE_FAILED;
            }
            
            is_response_ready = true;
            
        }break;
        case GET_FW_INFO_FROM_MAIN_FILE:
        {
            is_error = false;
            p_str = strstr(p_payload, NEW_FW_TOK);

            get_str_value(p_payload, new_fw_file_name, NEW_FW_TOK, "\r\n");
            get_str_value(p_payload, old_fw_file_name, OLD_FW_TOK, "\r\n");
            get_str_value(p_payload, current_fw_file_name, CURRENT_FW_TOK, "\r\n");
            get_str_value(p_payload, upgrade_restries_str, UPGRADE_RETRIES_TOK, "\r\n");

            if (new_fw_file_name[0] == NULL ||
                old_fw_file_name[0] == NULL ||
                current_fw_file_name[0] == NULL ||
                upgrade_restries_str[0] == NULL)
            {
                is_error = true;
            }

            for (uint32_t i = 0; i < strlen(upgrade_restries_str); i++)
            {
                if(upgrade_restries_str[i] < '0' || upgrade_restries_str[i] > '9')
                {
                    is_error= true;
                    break;
                }
            }

            if(!is_error)
            {
                upgrade_retries = atoi(upgrade_restries_str);
                state = GET_DAT_FILE_SIZE; 
            }
            else
            {
                state = UPGRADE_FAILED;
            }

            is_response_ready = true;
                        
        } break;
        case GET_DATA_FROM_DAT_FILE:
        {
            is_error = false;

            request.select.object_type = 0x01; //.dat file
            
            // Buffer will be freed asynchronously
            buf_free = false;

            request.request = NRF_DFU_OP_OBJECT_WRITE;

            request.write.p_data   = p_payload;
            request.write.len      = payload_len;
            request.callback.write = p_transport->payload_free_func;
            
            NRF_LOG_INFO("dat file info received*****************");
            NRF_LOG_HEXDUMP_DEBUG(p_payload, payload_len);
        }break;
        case GET_DAT_FILE_SIZE:
        case GET_FILE_SIZE:
        {
            is_error = false;
            index_tk = 0;
            remove_char((uint8_t *)p_payload, '\r');
            remove_char((uint8_t *)p_payload, '\n');
            tokens[index_tk++] = strtok((uint8_t *)p_payload, " ,");
            do
            {
                tokens[index_tk] = strtok(NULL, " ,");
            }while(tokens[index_tk++] != NULL);
            p_str = strstr(tokens[2],"OK");
            if(p_str != NULL)
            {
                *p_str = '\0';
            }
            else
            {
                is_error= true;
            }
            if(strcmp("+QFLST:", tokens[0]) == 0)
            {
                
                for (uint32_t i = 0; i < strlen(tokens[2]); i++)
                {
                    if(tokens[2][i] < '0' || tokens[2][i] > '9')
                    {
                        is_error= true;
                        break;
                    }
                }
                if (!is_error)
                {
                    switch(state)
                    {
                        case GET_DAT_FILE_SIZE:
                        {
                            dat_file_size = atoi(tokens[2]);
                            request.request = NRF_DFU_OP_OBJECT_CREATE;
        

                            request.create.object_type = 0x01; // .dat file
                            request.create.object_size = dat_file_size;

                            is_response_ready = true;
                            state = OPEN_DAT_FILE;
                        }break;
                        case GET_FILE_SIZE:
                        {
                            fw_file_size = atoi(tokens[2]);
                            //TODO: add validations to check if new FW will fit
                            set_file_size(fw_file_size);
                            is_response_ready = true;
                            state = OPEN_FW_FILE;
                        }break;
                        default:
                          break;
                    }
                    
                }
            }
            else
            {
                is_error = true;
            }
            
        }break;
        case OPEN_DAT_FILE:
        case OPEN_FW_FILE:
        {
            is_error = false;
            index_tk = 0;
            remove_char((uint8_t *)p_payload, '\r');
            remove_char((uint8_t *)p_payload, '\n');
            tokens[index_tk++] = strtok((uint8_t *)p_payload, " ");
            do
            {
                tokens[index_tk] = strtok(NULL, " ");
            }while(tokens[index_tk++] != NULL);
            p_str = strstr(tokens[1],"OK");

            if(p_str != NULL)
            {
                *p_str = '\0';
                if(strcmp("+QFOPEN:", tokens[0]) == 0)
                {
                
                    for (uint32_t i = 0; i < strlen(tokens[1]); i++)
                    {
                        if(tokens[1][i] < '0' || tokens[1][i] > '9')
                        {
                            is_error= true;
                            break;
                        }
                    }
                    if (!is_error)
                    {
                        switch(state)
                        {
                            case OPEN_DAT_FILE:
                            {
                                dat_file_handle = atoi(tokens[1]);
                                
                                state = GET_DATA_FROM_DAT_FILE;
                            }break;
                            case OPEN_FW_FILE:
                            {
                                fw_file_handle = atoi(tokens[1]);
                                
                                state = WRITE;
                            }break;
                            default:
                              break;
                        }
                    
                    }
                    else
                    {
                        state = UPGRADE_FAILED;
                    }
                }
                else
                {
                    state = UPGRADE_FAILED;
                }
            
            }
            else
            {
                state = UPGRADE_FAILED;
            }
            is_response_ready = true;
            
        }break;
        case CLOSE_MAIN_FILE:
        {
            is_error = false;
            remove_char((uint8_t *)p_payload, '\r');
            remove_char((uint8_t *)p_payload, '\n');
            p_str = strstr(p_payload,"OK");
            if(p_str != NULL)
            {
                is_main_file_open = false;
                state = CLOSE_DAT_FILE;
            }
            else
            {
                is_error= true;
                state = CLOSE_DAT_FILE;
            }
            is_response_ready = true;
            
        }break;
        case CLOSE_DAT_FILE:
        {
            is_error = false;
            remove_char((uint8_t *)p_payload, '\r');
            remove_char((uint8_t *)p_payload, '\n');
            p_str = strstr(p_payload,"OK");
            if(p_str != NULL)
            {
                is_dat_file_open = false;
                state = CLOSE_FW_FILE;
            }
            else
            {
                is_error= true;
                state = CLOSE_FW_FILE;
            }
            is_response_ready = true;
        }
        break;
        case CLOSE_FW_FILE:
        {
            is_error = false;
            remove_char((uint8_t *)p_payload, '\r');
            remove_char((uint8_t *)p_payload, '\n');
            p_str = strstr(p_payload,"OK");
            if(p_str != NULL)
            {
                is_fw_file_open = false;
                state = START_APP;
            }
            else
            {
                is_error= true;
                state = START_APP;
            }
            is_response_ready = true;
            
        }break;
        /*
        case UPGRADE_FAILED:
        {
            if (strstr(p_payload, "\nCONNECT\r\n") != NULL)
            {
                is_response_ready = true;
            }
            else
            {}
        }break;
        */
        case WRITE_UPGRADE_STATUS_SET_POINTER:
        {
            state = WRITE_UPGRADE_STATUS_TRUNCATE_FILE;
            is_response_ready = true;
        }break;
        case WRITE_UPGRADE_STATUS_TRUNCATE_FILE:
        {
            state = WRITE_UPGRADE_STATUS_SEND_CMD;
            is_response_ready = true;
        }break;
        case WRITE_UPGRADE_STATUS_SEND_CMD:
        {
            state = WRITE_UPGRADE_STATUS_SEND_DATA;
            is_response_ready = true;
        }break;
        case WRITE_UPGRADE_STATUS_SEND_DATA:
        {
            state = CLOSE_MAIN_FILE;
            is_response_ready = true;
        }break;

        default:
            /* Do nothing. */
            break;
    }

    if (buf_free)
    {
        memset((uint8_t*)p_data, 0x00, length);
        p_transport->payload_free_func((void *)(p_payload));
    }

    ret_code_t ret_code = nrf_dfu_req_handler_on_req(&request);
    ASSERT(ret_code == NRF_SUCCESS);
}
