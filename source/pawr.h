/******************************************************************************
* File Name:   pawr.h
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
typedef void (pawr_conn_up_cb_t)(const wiced_bt_ble_pawr_sync_established_event_data_t *pawr_param);
typedef void (pawr_conn_down_cb_t)(void);
void pawr_reg_se_rsp_cb(pawr_se_rsp_cb_t *callback);
void pawr_reg_conn_up_cb(pawr_conn_up_cb_t *callback);
void pawr_reg_conn_down_cb(pawr_conn_down_cb_t *callback);
wiced_bt_dev_status_t pawr_snd_se_rsp_central(uint16_t sync_handle,uint16_t evt_counter,uint8_t req_subevent,uint8_t rsp_subevent,uint8_t rsp_slot,uint8_t rsp_data_len,uint8_t *p_data);
void pawr_set_central_addr(const uint8_t *addr);
void pawr_scan_for_pawr_network(void);
void pawr_init(void);
#endif /* PAWR_H_ */

