/******************************************************************************
* File Name:   pawr.c
*
* Description: This file consists of the inteface for PAwR control.
*
* Related Document: See README.md
*
*
*******************************************************************************
* Copyright 2021-2024, Cypress Semiconductor Corporation (an Infineon company) or
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
#include "cyabs_rtos.h"
#include "cybsp.h"
#include "cyhal.h"
#include "wiced_bt_ble.h"
#include "wiced_bt_uuid.h"
#include "wiced_bt_trace.h"
#include "wiced_timer.h"
#include "wiced_memory.h"
#include "wiced_bt_cfg.h"
#include "cybt_platform_trace.h"
#include "pawr.h"
#ifdef ENABLE_BT_SPY_LOG
#include "cybt_debug_uart.h"
#else
#include "cy_retarget_io.h"
#endif
#include "app_bt_utils.h"

/*******************************************************************************
* Macro Definitions
*******************************************************************************/

/*******************************************************************************
* Variable Definitions
*******************************************************************************/
static uint16_t     pawr_conn_handle                  = 0xFFFF;
static uint8_t      pawr_central_address[BD_ADDR_LEN] = {0x00};
pawr_se_rsp_cb_t    * pawr_se_rsp_cb                  = NULL;
pawr_conn_up_cb_t   * pawr_conn_up_cb                 = NULL;
pawr_conn_down_cb_t * pawr_conn_down_cb               = NULL;
uint8_t             subevents[2]                      = {0, 1};

/******************************************************************************
* Function Definitions
******************************************************************************/
/**************************************************************************************************
* Function Name: pawr_snd_se_rsp_central()
***************************************************************************************************
* Function Description:
* @brief
* This function send the subevent response data to central.
* @param[in] sync_handle  ,      handle for synchronized advertising train.
* @param[in] subevent_num ,      PAwR response subevent.
* @param[in] response_slot,      PAwR response slot.
* @param[in] data_len     ,      PAwR response data len.
* @param[in] data         ,      PAwR response data point.
* @return void.
**************************************************************************************************/
wiced_bt_dev_status_t pawr_snd_se_rsp_central(uint16_t sync_handle,
                                              uint16_t evt_counter,
                                              uint8_t req_subevent,
                                              uint8_t rsp_subevent,
                                              uint8_t rsp_slot,
                                              uint8_t rsp_data_len,
                                              uint8_t *p_data)
{
    wiced_bt_ble_pawr_subevent_rsp_data_t pawr_subevent_rsp_data;
    pawr_subevent_rsp_data.req_event    = evt_counter;
    pawr_subevent_rsp_data.req_subevent = req_subevent;
    pawr_subevent_rsp_data.rsp_subevent = rsp_subevent;
    pawr_subevent_rsp_data.rsp_slot     = rsp_slot;
    pawr_subevent_rsp_data.rsp_data_len = rsp_data_len;
    pawr_subevent_rsp_data.p_data       = p_data;
    return wiced_bt_ble_set_pawr_subevent_rsp_data(sync_handle, &pawr_subevent_rsp_data);
}

/**************************************************************************************************
* Function Name: pawr_scan_for_pawr_network()
***************************************************************************************************
* Function Description:
* @brief
* This function scan for a PAwR network.
* @param[in] void.
* @return    void.
**************************************************************************************************/
void pawr_scan_for_pawr_network(void)
{
    wiced_bt_dev_status_t           status = WICED_BT_ERROR;
    wiced_bt_ble_ext_scan_config_t  scan_cfg;
    scan_cfg.scanning_phys = WICED_BT_BLE_EXT_ADV_PHY_1M;
    scan_cfg.duration      = 0;
    scan_cfg.period        = 0;
    /* set extended scan parameters */
    status = wiced_bt_ble_cache_ext_scan_config(&scan_cfg);
    printf("wiced_bt_ble_cache_ext_scan_config:result:%d\n",status);
    /* set extended scan enable */
    status = wiced_bt_ble_scan(BTM_BLE_SCAN_TYPE_HIGH_DUTY, WICED_FALSE, NULL);
    printf("wiced_bt_ble_scan:result:%d\n",status);
    status = wiced_bt_ble_create_sync_to_periodic_adv(WICED_BT_BLE_IGNORE_SYNC_TO_PERIODIC_ADV_LIST,
                                                      EXT_ADV_SET_ID,
                                                      BLE_ADDR_PUBLIC,
                                                      pawr_central_address,
                                                      0,
                                                      PERIODIC_ADV_EXPIRD_TIME,
                                                      0);
    printf("pawr_scan_for_pawr_network:addr: ");
    app_bt_util_print_bd_address(pawr_central_address);
    printf("pawr_scan_for_pawr_network status:%d\n", status);
    printf("pawr start\n");
}

/**************************************************************************************************
* Function Name: pawr_reg_se_rsp_cb()
***************************************************************************************************
* Function Description:
* @brief
* This function reg the PAwR sub event indication report callback.
* @param[in]       callback, PAwR sub event indication report callback.
* @return void.
**************************************************************************************************/
void pawr_reg_se_rsp_cb(pawr_se_rsp_cb_t *callback)
{
    printf("pawr_reg_subevt_rsp_cb\n");
    if (callback)
    {
        pawr_se_rsp_cb = callback;
    }
}

/**************************************************************************************************
* Function Name: pawr_inform_se_ind_app()
***************************************************************************************************
* Function Description:
* @brief
* This function inform the PAwR sub event indication report to app.
* @param[in] sync_handle  ,  Handle for synchronized advertising train.
* @param[in] p_msg        ,  Pointer to PAwR sub event indication report payload.
* @param[in] msg_len      ,  Length of PAwR sub event indication report.
* @param[in] subevent_num ,  The subevent number.
* @param[in] evt_counter  ,  Periodic_evt_counter.
* @return void.
*/
static void pawr_inform_se_ind_rcv_app(uint16_t sync_handle,uint8_t *p_msg, uint16_t msg_len, uint8_t subevent_num, uint16_t evt_counter)
{
    if (pawr_se_rsp_cb)
    {
        pawr_se_rsp_cb(sync_handle,p_msg,msg_len,subevent_num,evt_counter);
    }
}

/**************************************************************************************************
* Function Name: pawr_reg_conn_down_cb()
***************************************************************************************************
* Function Description:
* @brief
* This function reg the PAwR sync lost callback.
* @param[in] callback, callback function for PAwR sync lost.
* @return void.
**************************************************************************************************/
void pawr_reg_conn_down_cb(pawr_conn_down_cb_t *callback)
{
    printf("pawr_reg_conn_down_cb\n");
    if (callback)
    {
        pawr_conn_down_cb = callback;
    }
}

/**************************************************************************************************
* Function Name: pawr_inform_conn_down_app()
***************************************************************************************************
* Function Description:
* @brief
* This function is peripheral lose sync to central.
* @param[in]       void.
* @return          void.
**************************************************************************************************/
static void pawr_inform_conn_down_app(void)
{
    printf("pawr conn down\n");
    /* Disconnect the sync handle, and start scanning for sync again. */
    wiced_bt_ble_terminate_sync_to_periodic_adv(pawr_conn_handle);
    pawr_conn_handle = 0xFFFF;
    if (pawr_conn_down_cb)
    {
        pawr_conn_down_cb();
    }
}

/**************************************************************************************************
* Function Name: pawr_reg_conn_up_cb()
***************************************************************************************************
* Function Description:
* @brief
* This function reg PAwR sync up callback.
* @param[in] callback, callback function for PAwR sync up.
* @return void.
**************************************************************************************************/
void pawr_reg_conn_up_cb(pawr_conn_up_cb_t *callback)
{
    printf("pawr_reg_conn_up_cb\n");
    if (callback)
    {
        pawr_conn_up_cb = callback;
    }
}

/**************************************************************************************************
* Function Name: pawr_inform_conn_up_app()
***************************************************************************************************
* Function Description:
* @brief
* This function is peripheral synchronization to a central established.
* @param[in] ps, Supporting event data.
* @return    void.
**************************************************************************************************/
static void pawr_inform_conn_up_app(wiced_bt_ble_pawr_sync_established_event_data_t *ps)
{
    /* save the sync handle */
    pawr_conn_handle = ps->sync_handle;
    wiced_bt_ble_set_pawr_sync_subevents(pawr_conn_handle, 0, sizeof(subevents), subevents);

    /* stop scanning */
    wiced_bt_ble_scan(BTM_BLE_SCAN_TYPE_NONE, WICED_FALSE, NULL);
    if (pawr_conn_up_cb)
    {
        pawr_conn_up_cb(ps);
    }
}

/**************************************************************************************************
* Function Name: pawr_ext_adv_callback()
***************************************************************************************************
* Function Description:
* @brief
* This function process Extended ADV events.
* @param[in] event , The extended adv event code.
* @param[in] p_data, Event data refer to wiced_bt_ble_adv_ext_event_data_t.
* @return    void.
**************************************************************************************************/
static void pawr_ext_adv_callback(wiced_bt_ble_adv_ext_event_t event, wiced_bt_ble_adv_ext_event_data_t *p_data)
{
    switch (event)
    {
        case WICED_BT_BLE_PAWR_SUBEVENT_DATA_REQ_EVENT:
            printf("central sub event data ind\n");
        break;
        case WICED_BT_BLE_PAWR_RSP_REPORT_EVENT:
            printf("central sub event received data ind response report from peripheral\n");
        break;
        case WICED_BT_BLE_PERIODIC_ADV_SYNC_LOST_EVENT:
            pawr_inform_conn_down_app();
        break;
        case WICED_BT_BLE_PAWR_SYNC_ESTABLISHED_EVENT:
            pawr_inform_conn_up_app(&p_data->pawr_sync);
        break;
        case WICED_BT_BLE_PAWR_IND_REPORT_EVENT:
            if (p_data->pawr_ind_report.data_length != 0)
            {
                pawr_inform_se_ind_rcv_app(p_data->pawr_ind_report.sync_handle,
                                           p_data->pawr_ind_report.data,
                                           p_data->pawr_ind_report.data_length,
                                           p_data->pawr_ind_report.sub_event,
                                           p_data->pawr_ind_report.periodic_evt_counter);
            }
        break;
        default:
        break;
    }
}

/**************************************************************************************************
* Function Name: pawr_set_central_addr()
***************************************************************************************************
* Function Description:
* @brief
* This function set central adv address.
* @param[in] addr, app set central address.
* @return    void.
**************************************************************************************************/
void pawr_set_central_addr(const uint8_t *addr)
{
    memcpy(pawr_central_address,addr,6);
    printf("pawr_set_central_addr: ");
    app_bt_util_print_bd_address(pawr_central_address);
}

/**************************************************************************************************
* Function Name: pawr_init()
***************************************************************************************************
* Function Description:
* @brief
* This function initialize pwar peripheral.
* @param[in]       void.
* @return          void.
**************************************************************************************************/
void pawr_init(void)
{
    wiced_bt_ble_observe(WICED_FALSE, 0, NULL);
    wiced_bt_ble_register_adv_ext_cback(pawr_ext_adv_callback);
    pawr_scan_for_pawr_network();
}

/* [] END OF FILE */

