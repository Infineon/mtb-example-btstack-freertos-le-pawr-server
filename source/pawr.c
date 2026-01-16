/******************************************************************************
* File Name:   pawr.c
*
* Description: This file consists of the inteface for PAwR control.
*
* Related Document: See README.md
*
*
*******************************************************************************
 * (c) 2021-2026, Infineon Technologies AG, or an affiliate of Infineon
 * Technologies AG. All rights reserved.
 * This software, associated documentation and materials ("Software") is
 * owned by Infineon Technologies AG or one of its affiliates ("Infineon")
 * and is protected by and subject to worldwide patent protection, worldwide
 * copyright laws, and international treaty provisions. Therefore, you may use
 * this Software only as provided in the license agreement accompanying the
 * software package from which you obtained this Software. If no license
 * agreement applies, then any use, reproduction, modification, translation, or
 * compilation of this Software is prohibited without the express written
 * permission of Infineon.
 *
 * Disclaimer: UNLESS OTHERWISE EXPRESSLY AGREED WITH INFINEON, THIS SOFTWARE
 * IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING, BUT NOT LIMITED TO, ALL WARRANTIES OF NON-INFRINGEMENT OF
 * THIRD-PARTY RIGHTS AND IMPLIED WARRANTIES SUCH AS WARRANTIES OF FITNESS FOR A
 * SPECIFIC USE/PURPOSE OR MERCHANTABILITY.
 * Infineon reserves the right to make changes to the Software without notice.
 * You are responsible for properly designing, programming, and testing the
 * functionality and safety of your intended application of the Software, as
 * well as complying with any legal requirements related to its use. Infineon
 * does not guarantee that the Software will be free from intrusion, data theft
 * or loss, or other breaches ("Security Breaches"), and Infineon shall have
 * no liability arising out of any Security Breaches. Unless otherwise
 * explicitly approved by Infineon, the Software may not be used in any
 * application where a failure of the Product or any consequences of the use
 * thereof can reasonably be expected to result in personal injury.
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

wiced_ble_ext_scan_params_t scan_params =
{
    .own_addr_type = WICED_BLE_OWN_ADDR_PUBLIC,
    .scanning_phys = WICED_BLE_EXT_ADV_PHY_1M_BIT,
    .scan_filter_policy = WICED_BLE_EXT_SCAN_BASIC_UNFILTERED_SP,
    .sp_1m.scan_type = BTM_BLE_SCAN_MODE_PASSIVE,
    .sp_1m.scan_interval = WICED_BT_CFG_DEFAULT_HIGH_DUTY_SCAN_INTERVAL,
    .sp_1m.scan_window = WICED_BT_CFG_DEFAULT_HIGH_DUTY_SCAN_WINDOW
};

wiced_ble_ext_scan_enable_params_t scan_enable =
{
    .filter_duplicates = 0,
    .scan_duration = 0,
    .scan_period = 0,
};

wiced_ble_padv_create_sync_params_t sync_par =
{
    .options = WICED_BLE_PADV_CREATE_SYNC_OPTION_IGNORE_PA_LIST,
    .adv_sid = EXT_ADV_SET_ID,
    .adv_addr_type = BLE_ADDR_PUBLIC,
    .adv_addr = {0xC0, 0x01, 0x02, 0x03, 0x04, 0x05},
    .skip= 0,
    .sync_timeout = PERIODIC_ADV_EXPIRD_TIME,
    .sync_cte_type = 0,
};

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
    wiced_ble_padv_subevent_rsp_data_t pawr_subevent_rsp_data;
    pawr_subevent_rsp_data.req_event    = evt_counter;
    pawr_subevent_rsp_data.req_subevent = req_subevent;
    pawr_subevent_rsp_data.rsp_subevent = rsp_subevent;
    pawr_subevent_rsp_data.rsp_slot     = rsp_slot;
    pawr_subevent_rsp_data.rsp_data_len = rsp_data_len;
    pawr_subevent_rsp_data.p_data       = p_data;
    return wiced_ble_padv_set_subevent_rsp_data(sync_handle, &pawr_subevent_rsp_data);
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

    /* set extended scan parameters */
    status = wiced_ble_ext_scan_set_params(&scan_params);
    if (WICED_SUCCESS != status)
    {
        printf("Error setting extended scan parameters: %d\n", status);
        return;
    }

    /* Disable extended scan */
    status = wiced_ble_ext_scan_enable(0, &scan_enable);
    if (WICED_SUCCESS != status)
    {
        printf("Error disabling extended scan: %d\n", status);
        return;
    }

    memcpy(sync_par.adv_addr, pawr_central_address, BD_ADDR_LEN);

    status = wiced_ble_padv_create_sync(&sync_par);
    if (WICED_SUCCESS != status)
    {
        printf("Error create_sync: %d\n", status);
    }

    printf("pawr_scan_for_pawr_network:addr: ");
    app_bt_util_print_bd_address(pawr_central_address);

    /* set extended scan enable */
    status = wiced_ble_ext_scan_enable(1, &scan_enable);
    if (WICED_SUCCESS != status)
    {
        printf("Error starting extended scan: %d\n", status);
        return;
    }

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
* Function Name: pawr_inform_se_ind_rcv_app()
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
    wiced_ble_padv_terminate_sync(pawr_conn_handle);
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
static void pawr_inform_conn_up_app(wiced_ble_padv_sync_established_event_data_t *ps)
{
    /* save the sync handle */
    pawr_conn_handle = ps->sync_handle;
    wiced_ble_padv_set_sync_subevent(pawr_conn_handle, 0, sizeof(subevents), subevents);

    /* stop scanning */
    wiced_ble_ext_scan_enable(0, &scan_enable);
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
* @param[in] p_data, Event data refer to wiced_ble_ext_adv_event_data_t.
* @return    void.
**************************************************************************************************/
static void pawr_ext_adv_callback(wiced_ble_ext_adv_event_t event, wiced_ble_ext_adv_event_data_t *p_data)
{
    switch (event)
    {
        case WICED_BT_BLE_PAWR_SUBEVENT_DATA_REQ_EVENT:
            printf("central sub event data ind\n");
        break;
        case WICED_BT_BLE_PAWR_RSP_REPORT_EVENT:
            printf("central sub event received data ind response report from peripheral\n");
        break;
        case WICED_BLE_PERIODIC_ADV_SYNC_LOST_EVENT:
            pawr_inform_conn_down_app();
        break;
        case WICED_BLE_PERIODIC_ADV_SYNC_ESTABLISHED_EVENT:
            pawr_inform_conn_up_app(&p_data->sync_establish);
        break;
        case WICED_BLE_PERIODIC_ADV_REPORT_EVENT:
            if (p_data->periodic_adv_report.data_length != 0)
            {
                pawr_inform_se_ind_rcv_app(p_data->periodic_adv_report.sync_handle,
                                           p_data->periodic_adv_report.p_data,
                                           p_data->periodic_adv_report.data_length,
                                           p_data->periodic_adv_report.sub_event,
                                           p_data->periodic_adv_report.periodic_evt_counter);
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
    wiced_ble_ext_adv_register_cback(pawr_ext_adv_callback);
    pawr_scan_for_pawr_network();
}

/* [] END OF FILE */

