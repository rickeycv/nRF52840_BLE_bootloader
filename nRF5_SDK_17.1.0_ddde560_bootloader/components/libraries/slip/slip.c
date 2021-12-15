/**
 * Copyright (c) 2015 - 2021, Nordic Semiconductor ASA
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
#include "sdk_common.h"
#if NRF_MODULE_ENABLED(SLIP)
#include "slip.h"

#include <string.h>


#define SLIP_BYTE_END             0300    /* indicates end of packet */
#define SLIP_BYTE_ESC             0333    /* indicates byte stuffing */
#define SLIP_BYTE_ESC_END         0334    /* ESC ESC_END means END data byte */
#define SLIP_BYTE_ESC_ESC         0335    /* ESC ESC_ESC means ESC data byte */

static uint32_t frame_size;
extern uint8_t state;
extern bool is_response_ready;


ret_code_t slip_encode(uint8_t * p_output,  uint8_t * p_input, uint32_t input_length, uint32_t * p_output_buffer_length)
{
    if (p_output == NULL || p_input == NULL || p_output_buffer_length == NULL)
    {
        return NRF_ERROR_NULL;
    }

    *p_output_buffer_length = 0;
    uint32_t input_index;

    for (input_index = 0; input_index < input_length; input_index++)
    {
        switch (p_input[input_index])
        {
            case SLIP_BYTE_END:
                p_output[(*p_output_buffer_length)++] = SLIP_BYTE_ESC;
                p_output[(*p_output_buffer_length)++] = SLIP_BYTE_ESC_END;
                break;

            case SLIP_BYTE_ESC:
                p_output[(*p_output_buffer_length)++] = SLIP_BYTE_ESC;
                p_output[(*p_output_buffer_length)++] = SLIP_BYTE_ESC_ESC;
                break;

            default:
                p_output[(*p_output_buffer_length)++] = p_input[input_index];
        }
    }
    p_output[(*p_output_buffer_length)++] = SLIP_BYTE_END;

    return NRF_SUCCESS;
}

ret_code_t slip_decode_add_byte(slip_t * p_slip, uint8_t c)
{
    if (p_slip == NULL)
    {
        return NRF_ERROR_NULL;
    }

    if (p_slip->current_index == p_slip->buffer_len)
    {
        return NRF_ERROR_NO_MEM;
    }

    switch (p_slip->state)
    {
        case SLIP_STATE_DECODING:
            switch (c)
            {/*
                case SLIP_BYTE_END:
                    // finished reading packet
                    return NRF_SUCCESS;

                case SLIP_BYTE_ESC:
                    // wait for
                    p_slip->state = SLIP_STATE_ESC_RECEIVED;
                    break;
*/
                default:
                    // add byte to buffer
                    p_slip->p_buffer[p_slip->current_index++] = c;
                    switch(state)
                    {
                        case RESET_MODEM:
                        {
                            uint8_t cont_ocurrences = 0;
                            uint8_t* substr = "RDY\r\n";
                            uint8_t* p_str = p_slip->p_buffer;
                            
                            do
                            {
                                p_str = strstr(p_str, substr);
                                if (p_str != NULL)
                                {
                                    return NRF_SUCCESS;
                                }
                            }while(p_str != NULL);
                        }break;
                        case GET_MAIN_FILE_SIZE:
                        case OPEN_MAIN_FILE:
                        case GET_DAT_FILE_SIZE:
                        case OPEN_DAT_FILE:
                        case GET_FILE_SIZE:
                        case OPEN_FW_FILE:
                        {
                            uint8_t cont_ocurrences = 0;
                            uint8_t* substr = "\r\n";
                            uint8_t* p_str = p_slip->p_buffer;
                            
                            do
                            {
                                p_str = strstr(p_str, substr);
                                if (p_str != NULL)
                                {
                                    cont_ocurrences++;
                                    p_str += strlen(substr);
                                }
                            }while(p_str != NULL);
                            
                            // If number of ocurrences are not met, timeout will occur
                            if (cont_ocurrences == 4)
                            {
                                return NRF_SUCCESS;
                            }
                        }
                        break;
                        case GET_FW_INFO_FROM_MAIN_FILE:
                        case GET_DATA_FROM_DAT_FILE:
                        case WRITE:
                        {
                            if (p_slip->current_index >= frame_size)
                            {
                              return NRF_SUCCESS;
                            }
                        }
                        break;
                        case INIT:
                        case CLOSE_MAIN_FILE:
                        case CLOSE_DAT_FILE:
                        case CLOSE_FW_FILE:
                        case WRITE_UPGRADE_STATUS_SET_POINTER:
                        case WRITE_UPGRADE_STATUS_TRUNCATE_FILE:
                        case WRITE_UPGRADE_STATUS_SEND_CMD:
                        {
                            uint8_t cont_ocurrences = 0;
                            uint8_t* substr = "\r\n";
                            uint8_t* p_str = p_slip->p_buffer;
                            
                            do
                            {
                                p_str = strstr(p_str, substr);
                                if (p_str != NULL)
                                {
                                    cont_ocurrences++;
                                    p_str += strlen(substr);
                                }
                            }while(p_str != NULL);
                            
                            // If number of ocurrences are not met, timeout will occur
                            if (cont_ocurrences == 2)
                            {
                                return NRF_SUCCESS;
                            }
                        }break;
                        case WRITE_UPGRADE_STATUS_SEND_DATA:
                        {
                            uint8_t cont_ocurrences = 0;
                            uint8_t* substr = "\r\n";
                            uint8_t* p_str = p_slip->p_buffer;
                            
                            do
                            {
                                p_str = strstr(p_str, substr);
                                if (p_str != NULL)
                                {
                                    cont_ocurrences++;
                                    p_str += strlen(substr);
                                }
                            }while(p_str != NULL);
                            
                            // If number of ocurrences are not met, timeout will occur
                            if (cont_ocurrences == 3)
                            {
                                return NRF_SUCCESS;
                            }
                        }break;
                        default:
                        break;
                    }

                    break;
            }
            break;

        case SLIP_STATE_ESC_RECEIVED:
            switch (c)
            {
                case SLIP_BYTE_ESC_END:
                    p_slip->p_buffer[p_slip->current_index++] = SLIP_BYTE_END;
                    p_slip->state = SLIP_STATE_DECODING;
                    break;

                case SLIP_BYTE_ESC_ESC:
                    p_slip->p_buffer[p_slip->current_index++] = SLIP_BYTE_ESC;
                    p_slip->state = SLIP_STATE_DECODING;
                    break;

                default:
                    // protocol violation
                    p_slip->state = SLIP_STATE_CLEARING_INVALID_PACKET;
                    return NRF_ERROR_INVALID_DATA;
            }
            break;

        case SLIP_STATE_CLEARING_INVALID_PACKET:
            if (c == SLIP_BYTE_END)
            {
                p_slip->state = SLIP_STATE_DECODING;
                p_slip->current_index = 0;
            }
            break;
    }

    return NRF_ERROR_BUSY;
}

void set_frame_size(uint32_t _frame_size)
{
    frame_size = _frame_size;
}

#endif //NRF_MODULE_ENABLED(SLIP)
