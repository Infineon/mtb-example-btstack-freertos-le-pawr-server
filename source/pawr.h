/******************************************************************************
* File Name:   pawr.h
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

#ifndef PAWR_H_
#define PAWR_H_
/******************************************************************************
* Header Files
*******************************************************************************/
#include "wiced_bt_ble.h"

/*******************************************************************************
* Macro Definitions
*******************************************************************************/
#define EXT_ADV_SET_ID                  (0x00)   /* extended adv set id */
#define PERIODIC_ADV_EXPIRD_TIME        (1000)   /* 10 second 1000*10ms */

/*******************************************************************************
 * Variable Definitions
*******************************************************************************/

/******************************************************************************
 * Function Prototypes
*******************************************************************************/
typedef void (pawr_se_rsp_cb_t)(uint16_t sync_handle, uint8_t *p_msg, uint16_t msg_len, uint8_t subevent_num, uint16_t evt_counter);
typedef void (pawr_conn_up_cb_t)(const wiced_ble_padv_sync_established_event_data_t *pawr_param);
typedef void (pawr_conn_down_cb_t)(void);
void pawr_reg_se_rsp_cb(pawr_se_rsp_cb_t *callback);
void pawr_reg_conn_up_cb(pawr_conn_up_cb_t *callback);
void pawr_reg_conn_down_cb(pawr_conn_down_cb_t *callback);
wiced_bt_dev_status_t pawr_snd_se_rsp_central(uint16_t sync_handle,uint16_t evt_counter,uint8_t req_subevent,uint8_t rsp_subevent,uint8_t rsp_slot,uint8_t rsp_data_len,uint8_t *p_data);
void pawr_set_central_addr(const uint8_t *addr);
void pawr_scan_for_pawr_network(void);
void pawr_init(void);
#endif /* PAWR_H_ */

