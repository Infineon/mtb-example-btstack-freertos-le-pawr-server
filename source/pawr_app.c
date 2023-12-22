/******************************************************************************
* File Name:   pawr_app.c
*
* Description: This file consists of the inteface for PAwR config and data transfer.
*
* Related Document: See README.md
*
*
*******************************************************************************
* Copyright 2021-2023, Cypress Semiconductor Corporation (an Infineon company) or
* an affiliate of Cypress Semiconductor Corporation.  All rights reserved.
*
* This software, including source code, documentation and related
* materials ("Software") is owned by Cypress Semiconductor Corporation
* or one of its affiliates ("Cypress") and is protected by and subject to
* worldwide patent protection (United States and foreign),
* United States copyright laws and international treaty provisions.
* Therefore, you may use this Software only as provided in the license
* agreement accompanying the software package from which you
* obtained this Software ("EULA").
* If no EULA applies, Cypress hereby grants you a personal, non-exclusive,
* non-transferable license to copy, modify, and compile the Software
* source code solely for use in connection with Cypress's
* integrated circuit products.  Any reproduction, modification, translation,
* compilation, or representation of this Software except as specified
* above is prohibited without the express written permission of Cypress.
*
* Disclaimer: THIS SOFTWARE IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, NONINFRINGEMENT, IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. Cypress
* reserves the right to make changes to the Software without notice. Cypress
* does not assume any liability arising out of the application or use of the
* Software or any product or circuit described in the Software. Cypress does
* not authorize its products for use in any products where a malfunction or
* failure of the Cypress product may reasonably be expected to result in
* significant property damage, injury or death ("High Risk Product"). By
* including Cypress's product in a High Risk Product, the manufacturer
* of such system or application assumes all risk of such use and in doing
* so agrees to indemnify Cypress against all liability.
*******************************************************************************/

/*******************************************************************************
* Header Files
*******************************************************************************/
#include <string.h>
#include <FreeRTOS.h>
#include <string.h>
#include "cybsp.h"
#include "cyhal.h"
#include "cyabs_rtos.h"
#include "cybt_result.h"
#include "cybt_platform_trace.h"
#include "cybsp_bt_config.h"
#include "wiced_bt_cfg.h"
#include "wiced_memory.h"
#include "wiced_timer.h"
#include "wiced_bt_trace.h"
#include "wiced_bt_ble.h"
#include "wiced_bt_cfg.h"
#include "wiced_bt_stack.h"
#include "wiced_bt_dev.h"
#include "pawr.h"
#include "pawr_app.h"
#ifdef ENABLE_BT_SPY_LOG
#include "cybt_debug_uart.h"
#else
#include "cy_retarget_io.h"
#endif
#include "app_bt_utils.h"
#include "wiced_bt_gatt.h"
#include "app_bt_event_handler.h"

#include "hcidefs.h"
#include "wiced_bt_types.h"
#include "cycfg_bt_settings.h"

/*******************************************************************************
* Macro Definitions
*******************************************************************************/

/*******************************************************************************
* Variable Definitions
*******************************************************************************/
extern const char brcm_patch_version[];
static const char pawr_version[]                        = {"[1.00]"};
static uint8_t app_central_address[BD_ADDR_LEN]         = {0xc0,0x01,0x02,0x03,0x04,0x05};
static uint8_t app_peripheral_address[BD_ADDR_LEN]      = {0xc0,0x00,0x00,0x01,0x00,PAWR_PERIPHERAL_RSP_SLOT};
static const uint8_t pawr_subevent0_data[PAWR_BUF_SIZE] = {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f};
static const uint8_t pawr_subevent1_data[PAWR_BUF_SIZE] = {0x00,0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,0x99,0xaa,0xbb,0xcc,0xdd,0xee,0xff};

/******************************************************************************
* Function Definitions
******************************************************************************/
/**************************************************************************************************
* Function Name: hci_trace_cback()
***************************************************************************************************
* Function Description:
* @brief
* This callback routes HCI packets to debug uart.
* @param type   , HCI trace type.
* @param ength  , length of hci trace data.
* @param p_data , pointer to data.
* @return void
**************************************************************************************************/
#ifdef ENABLE_BT_SPY_LOG
void hci_trace_cback(wiced_bt_hci_trace_type_t type,
                     uint16_t length, uint8_t* p_data)
{
    cybt_debug_uart_send_hci_trace(type, length, p_data);
}
#endif

/**************************************************************************************************
* Function Name: app_pawr_subevt_rsp_cb()
***************************************************************************************************
* Function Description:
* @brief
* This function is the callback of received indication report from central.
* @param[in] sync_handle  , handle for synchronized advertising train.
* @param[in] p_msg        , PAwR sub event indication report data.
* @param[in] msg_len      , PAwR sub event indication report data len.
* @param[in] subevent_num , PAwR sub event indication report subevent.
* @param[in] evt_counter  , periodic_evt_counter.
* @return void
**************************************************************************************************/
void app_pawr_se_rsp_cb(uint16_t sync_handle, uint8_t *p_msg, uint16_t msg_len, uint8_t subevent_num, uint16_t evt_counter)
{
    wiced_bt_dev_status_t  status      = WICED_BT_ERROR;
    static uint32_t        rcv_se0_cnt = 0;
    static uint32_t        rcv_se1_cnt = 0;
    uint8_t rcv_buf[PAWR_BUF_SIZE];
    uint8_t *rcv = rcv_buf;
    uint8_t snd_buf[PAWR_BUF_SIZE];
    uint8_t *snd = snd_buf;
    if (((subevent_num == SUBEVT0) || (subevent_num == SUBEVT1)) && (msg_len == PAWR_BUF_SIZE))
    {
        switch(subevent_num)
        {
            case SUBEVT0:
                printf("rcv:se:%d,cnt:%lu,\n",subevent_num,rcv_se0_cnt++);
                if (memcmp(p_msg,pawr_subevent0_data,PAWR_BUF_SIZE))
                {
                    printf("se0 error:\n");
                    app_bt_util_print_byte_array(p_msg,msg_len);
                    return;
                }
            break;
            case SUBEVT1:
                printf("rcv:se:%d,cnt:%lu\n",subevent_num,rcv_se1_cnt++);
                if (memcmp(p_msg,pawr_subevent1_data,PAWR_BUF_SIZE))
                {
                    printf("se1 error:\n");
                    app_bt_util_print_byte_array(p_msg,msg_len);
                    return;
                }
            break;
        }
        STREAM_TO_ARRAY(rcv,p_msg,PAWR_BUF_SIZE);
        ARRAY_TO_STREAM(snd,rcv_buf,PAWR_BUF_SIZE);
        status = pawr_snd_se_rsp_central(sync_handle,
                                         evt_counter,
                                         subevent_num,
                                         subevent_num,
                                         PAWR_PERIPHERAL_RSP_SLOT,
                                         PAWR_BUF_SIZE,
                                         snd_buf);
        if (status != WICED_SUCCESS)
        {
            printf("pawr snd rsp error:%d\n",status);
            return;
        }
    }
}

/**************************************************************************************************
* Function Name: app_pawr_conn_up_cb()
***************************************************************************************************
* Function Description:
* @brief
* This function is PAwR sync connect up callback.
* @param[in] pawr_param @refer wiced_bt_ble_pawr_sync_established_event_data_t
* @return void
**************************************************************************************************/
void app_pawr_conn_up_cb(const wiced_bt_ble_pawr_sync_established_event_data_t *pawr_param)
{
    printf("pawr conn up: central_addr: ");
    app_bt_util_print_bd_address(pawr_param->adv_addr);
    printf("status:%d, snyc_hdl:0x%04x, adv_int:%d, sub_num:%d, sub_int:%d, slot_delay:%d, slot_sp:%d\n",
           pawr_param->status,
           pawr_param->sync_handle,
           pawr_param->periodic_adv_int,
           pawr_param->num_subevents,
           pawr_param->subevent_interval,
           pawr_param->response_slot_delay,
           pawr_param->response_slot_spacing);
}

/**************************************************************************************************
* Function Name: app_pawr_conn_down_cb()
***************************************************************************************************
* Function Description:
* @brief
* This function is PAwR sync connect down callback.
* @param[in] void
* @return    void
**************************************************************************************************/
void app_pawr_conn_down_cb(void)
{
    printf("pawr conn down\n");
    pawr_scan_for_pawr_network();
}

/**************************************************************************************************
* Function Name: app_peripheral_init()
***************************************************************************************************
* Function Description:
* @brief
* This function init PAwR peripheral. register PAwR callback, set PAwR central addr.
* @param[in] void.
* @return    void.
**************************************************************************************************/
void app_peripheral_init(void)
{
    printf("===================================\n");
    pawr_reg_se_rsp_cb(app_pawr_se_rsp_cb);
    pawr_reg_conn_up_cb(app_pawr_conn_up_cb);
    pawr_reg_conn_down_cb(app_pawr_conn_down_cb);
    printf("FW VERSION:%s\n",brcm_patch_version);
    printf("PAWR PERIPHERAL VERSION:%s\n",pawr_version);
    wiced_bt_set_local_bdaddr(app_peripheral_address, BLE_ADDR_PUBLIC);
    wiced_bt_dev_read_local_addr(app_peripheral_address);
    printf("central addr: ");
    app_bt_util_print_bd_address(app_central_address);
    printf("peripheral addr: ");
    app_bt_util_print_bd_address(app_peripheral_address);
#ifdef ENABLE_BT_SPY_LOG
    wiced_bt_dev_register_hci_trace(hci_trace_cback);
#endif
    pawr_set_central_addr((const uint8_t *)app_central_address);
    pawr_init();
    printf("===================================\n");
}
/* [] END OF FILE */

