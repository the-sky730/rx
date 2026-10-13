/*********************************************************************************************
    *   Filename        : btctrler_config.c

    *   Description     : Optimized Code & RAM (编译优化配置)

    *   Author          : Bingquan

    *   Email           : caibingquan@zh-jieli.com

    *   Last modifiled  : 2019-03-16 11:49

    *   Copyright:(c)JIELI  2011-2019  @ , All Rights Reserved.
*********************************************************************************************/
#include "app_config.h"
#include "system/includes.h"
#include "btcontroller_config.h"
#include "bt_common.h"

/**
 * @brief Bluetooth Module
 */

#if TCFG_USER_TWS_ENABLE

const int config_btctler_modules        = (BT_MODULE_CLASSIC | BT_MODULE_LE);
const int config_btctler_le_tws         = 1;
const int CONFIG_BTCTLER_TWS_ENABLE     = 1;
const int CONFIG_TWS_AFH_ENABLE         = 1;
const int CONFIG_TWS_POWER_BALANCE_ENABLE = 0;
const int CONFIG_LOW_LATENCY_ENABLE = 0;
const int ble_disable_wait_enable = 0;
const int CONFIG_BTCTLER_FAST_CONNECT_ENABLE     = 0;

#else

const int config_btctler_modules        =     0
#if (TCFG_USER_BT_CLASSIC_ENABLE)
        | BT_MODULE_CLASSIC
#endif//TCFG_USER_BLE_ENABLE
#if (TCFG_USER_BLE_ENABLE)
        | BT_MODULE_LE
#endif//TCFG_USER_BLE_ENABLE
        ;

const int config_btctler_le_tws         = 0;
const int CONFIG_BTCTLER_TWS_ENABLE     = 0;
const int CONFIG_TWS_AFH_ENABLE         = 0;
const int CONFIG_LOW_LATENCY_ENABLE     = 0;
const int ble_disable_wait_enable = 1;
const int CONFIG_BTCTLER_FAST_CONNECT_ENABLE     = 0;

#endif

const int CONFIG_BTCTLER_QOS_ENABLE         = 1;

const int CONFIG_PAGE_POWER                 = 4;
const int CONFIG_PAGE_SCAN_POWER            = 7;
const int CONFIG_INQUIRY_POWER              = 7;
const int CONFIG_INQUIRY_SCAN_POWER         = 7;


const int CONFIG_TWS_SUPER_TIMEOUT      = 4000;

#if (CONFIG_BT_MODE != BT_NORMAL)
const int config_btctler_hci_standard   = 1;
#else
const int config_btctler_hci_standard   = 0;
#endif

const int config_btctler_mode        = CONFIG_BT_MODE;

//固定使用正常发射功率的等级:0-使用不同模式的各自等级;1~10-固定发射功率等级
const int config_force_bt_pwr_tab_using_normal_level  = 0;
#if (WIRELESS_MICROPHONE_MODE)
const int CONFIG_BLE_SYNC_WORD_BIT = 28;
#else
const int CONFIG_BLE_SYNC_WORD_BIT = 30;
#endif
const int CONFIG_LNA_CHECK_VAL = -60;
const int CONFIG_DUT_POWER                  = 10;
/*-----------------------------------------------------------*/

/**
 * @brief Bluetooth Classic setting
 */
const u8 rx_fre_offset_adjust_enable = 1;

const int config_bredr_fcc_fix_fre = 0;

const int config_btctler_eir_version_info_len = 0;

const int config_delete_link_key          = 1;           //配置是否连接失败返回PIN or Link Key Missing时删除linkKey

#ifdef CONFIG_SOUNDBOX_FLASH_256K
const int CONFIG_TEST_DUT_CODE            = 1;
const int CONFIG_TEST_FCC_CODE            = 0;
const int CONFIG_TEST_DUT_ONLY_BOX_CODE   = 0;
#else
const int CONFIG_TEST_DUT_CODE            = 1;
const int CONFIG_TEST_FCC_CODE            = 1;
const int CONFIG_TEST_DUT_ONLY_BOX_CODE   = 0;
#endif


#ifdef CONFIG_SOUNDBOX_FLASH_256K
const int CONFIG_BREDR_INQUIRY   =  0;
#else
const int CONFIG_BREDR_INQUIRY   =  1;
#endif

const int CONFIG_ESCO_MUX_RX_BULK_ENABLE  =  0;
const int CONFIG_INQUIRY_PAGE_OFFSET_ADJUST =  0;

const int CONFIG_LMP_NAME_REQ_ENABLE  =  1;
const int CONFIG_LMP_PASSKEY_ENABLE  =  1;
const int CONFIG_LMP_MASTER_ESCO_ENABLE  =  1;
#if TCFG_WIFI_DETECT_ENABLE
const int CONFIG_WIFI_DETECT_ENABLE = 2;   //0:disable, 1:classic wifidetect, 2:(only latency==0)ble wifidetect
#else
const int CONFIG_WIFI_DETECT_ENABLE = 0;   //0:disable, 1:classic wifidetect, 2:(only latency==0)ble wifidetect
#endif
const int ESCO_FORWARD_ENABLE = 0;

const int config_bt_function  =  BT_ENCTRY_TASK  ;


#if ((AUDIO_OUTPUT_WAY == AUDIO_OUTPUT_WAY_BT) && (TCFG_BD_NUM == 2))
const int config_btctler_bredr_master = 1;
#if USER_SUPPORT_DUAL_A2DP_SOURCE
const int config_btctler_dual_a2dp  = 1;
#else
const int config_btctler_dual_a2dp  = 0;
#endif
#else
///bredr 强制 做 maseter
const int config_btctler_bredr_master = 1;
const int config_btctler_dual_a2dp  = 0;
#endif

///afh maseter 使用app设置的map 通过USER_CTRL_AFH_CHANNEL 设置
const int config_bredr_afh_user = 0;
//bt PLL 温度跟随trim
const int config_bt_temperature_pll_trim = 0;
/*security check*/
const int config_bt_security_vulnerability = 0;


/*-----------------------------------------------------------*/

/**
 * @brief Bluetooth LE setting
 */

#if TCFG_USER_BLE_ENABLE

const uint32_t config_vendor_le_bb_slots    = 0;
#if WIRELESS_CODING_FRAME_LEN == 100
const uint8_t config_vendor_le_bb_rtn       = 3;
#else
const uint8_t config_vendor_le_bb_rtn       = 0;
#endif

/* ---------------------------------- */
/* ---------------------------------- DEF_BLE_DEMO_MULTI */
#if (TCFG_BLE_DEMO_SELECT == DEF_BLE_DEMO_MULTI)

#if TRANS_MULTI_BLE_SM_ENABLE
#define SET_ENCRYPTION_CFG   LE_ENCRYPTION
#else
#define SET_ENCRYPTION_CFG   0
#endif

#if TRANS_MULTI_BLE_SLAVE_NUMS
#define SET_SLAVE_ROLS_CFG   (LE_ADV | LE_SLAVE)
#else
#define SET_SLAVE_ROLS_CFG   0
#endif

#if TRANS_MULTI_BLE_MASTER_NUMS
#define SET_MASTER_ROLS_CFG   (LE_SCAN | LE_INIT | LE_MASTER)
#else
#define SET_MASTER_ROLS_CFG   0
#endif
const int config_btctler_le_roles         = (LE_ADV | LE_SLAVE) | (LE_SCAN | LE_INIT | LE_MASTER);

#if (APP_MAIN == APP_WIRELESS_MIC_2T1)
const uint64_t config_btctler_le_features = LE_ENCRYPTION |
        LE_DATA_PACKET_LENGTH_EXTENSION |
        /* #if WIRELESS_HIGH_BW_EN */
        LE_2M_PHY |
#if TCFG_2T1_RX_PRODUCT_TEST_EN
        LE_CORE_V50_FEATURES |
#endif
        /* #endif #<{(| WIRELESS_HIGH_BW_EN |)}># */
        0;

u32 config_vendor_le_bb = VENDOR_BB_MD_CLOSE |
                          VENDOR_BB_CONNECT_SLOT |
                          VENDOR_BB_ADV_PDU_INT(3) |
                          VENDOR_BB_RX_PAYLOAD_LEN(TCFG_SERVER_RX_PAYLOAD_LEN) |
                          VENDOR_BB_NEW_SCAN_STRATEGY |
                          VENDOR_BB_TS |
#if WIRELESS_HIGH_BW_EN
                          VENDOR_BB_HIGH_BW |
#endif /* WIRELESS_HIGH_BW_EN */
                          0;
#else /* APP_MAIN == APP_WIRELESS_MIC_2T1 */
const uint64_t config_btctler_le_features = LE_ENCRYPTION | LE_DATA_PACKET_LENGTH_EXTENSION | LE_2M_PHY;
u32 config_vendor_le_bb = 0;
#endif /* APP_MAIN == APP_WIRELESS_MIC_2T1 */

#if WIRELESS_2t1_RX_ONLY_CONN_1TX
const int config_btctler_le_master_multilink = 0; // Master multi-link
#else
const int config_btctler_le_master_multilink = 1; // Master multi-link
#endif

#if TCFG_WIFI_DETECT_ENABLE
const int config_btctler_le_afh_en = 0; // Master AFH
#else /* TCFG_WIFI_DETECT_ENABLE */
const int config_btctler_le_afh_en = 1; // Master AFH
#endif /* TCFG_WIFI_DETECT_ENABLE */

/* LE RAM Control */
#if TCFG_2T1_RX_PRODUCT_TEST_EN
const int config_btctler_le_hw_nums           = 4;
#else
const int config_btctler_le_hw_nums           = 2;
#endif

const int config_btctler_le_rx_nums           = 12;
const int config_btctler_le_acl_packet_length = 27;
const int config_btctler_le_acl_total_nums    = 12;

/* ---------------------------------- */
/* ---------------------------------- DEF_BLE_DEMO_WIRELESS_MIC_CLIENT */
#elif (TCFG_BLE_DEMO_SELECT == DEF_BLE_DEMO_WIRELESS_MIC_CLIENT)

const int config_btctler_le_roles         = (LE_ADV | LE_SLAVE | LE_SCAN | LE_INIT | LE_MASTER);
const uint64_t config_btctler_le_features = LE_ENCRYPTION | LE_DATA_PACKET_LENGTH_EXTENSION | LE_2M_PHY;
u32 config_vendor_le_bb = VENDOR_BB_MD_CLOSE |
                          VENDOR_BB_CONNECT_SLOT |
                          VENDOR_BB_RX_PAYLOAD_LEN(TCFG_SERVER_RX_PAYLOAD_LEN) |
                          VENDOR_BB_TS |
                          0;

const int config_btctler_le_master_multilink = 0; // Master multi-link

#if TCFG_WIFI_DETECT_ENABLE
const int config_btctler_le_afh_en = 0; // Master AFH
#else /* TCFG_WIFI_DETECT_ENABLE */
const int config_btctler_le_afh_en = 1; // Master AFH
#endif /* TCFG_WIFI_DETECT_ENABLE */

/* LE RAM Control */
const int config_btctler_le_hw_nums           = 1;
const int config_btctler_le_rx_nums           = 8;
const int config_btctler_le_acl_packet_length = 251;
const int config_btctler_le_acl_total_nums    = 2;

/* ---------------------------------- */
/* ---------------------------------- DEF_BLE_DEMO_WIRELESS_MIC_SERVER */
#elif (TCFG_BLE_DEMO_SELECT == DEF_BLE_DEMO_WIRELESS_MIC_SERVER)

const int config_btctler_le_roles         = (LE_ADV | LE_SLAVE);
#if (APP_MAIN == APP_WIRELESS_MIC_2T1)
const uint64_t config_btctler_le_features = LE_ENCRYPTION |
        LE_DATA_PACKET_LENGTH_EXTENSION |
        /* #if WIRELESS_HIGH_BW_EN */
        LE_2M_PHY |
        /* #endif #<{(| WIRELESS_HIGH_BW_EN |)}># */
        0;
#else
const uint64_t config_btctler_le_features = LE_ENCRYPTION | LE_DATA_PACKET_LENGTH_EXTENSION | LE_2M_PHY;
#endif /* (APP_MAIN == APP_WIRELESS_MIC_2T1) */

u32 config_vendor_le_bb = VENDOR_BB_MD_CLOSE |
                          VENDOR_BB_CONNECT_SLOT |
                          VENDOR_BB_ADV_PDU_INT(3) |
                          VENDOR_BB_RX_PAYLOAD_LEN(TCFG_SERVER_RX_PAYLOAD_LEN) |
                          VENDOR_BB_TS |
#if WIRELESS_HIGH_BW_EN
                          VENDOR_BB_HIGH_BW |
#endif /* WIRELESS_HIGH_BW_EN */
                          0;

const int config_btctler_le_master_multilink = 0; // Master multi-link
const int config_btctler_le_afh_en           = 0; // Master AFH
/* LE RAM Control */
const int config_btctler_le_hw_nums           = 1;
const int config_btctler_le_rx_nums           = 8;
const int config_btctler_le_acl_packet_length = 251;
const int config_btctler_le_acl_total_nums    = 2;

/* ---------------------------------- */
/* ---------------------------------- DEF_BLE_DEMO_CLIENT */
#elif (TCFG_BLE_DEMO_SELECT == DEF_BLE_DEMO_CLIENT)

const int config_btctler_le_roles            = (LE_SCAN | LE_INIT | LE_MASTER);
const uint64_t config_btctler_le_features    = LE_ENCRYPTION;
u32 config_vendor_le_bb                = 0;
const int config_btctler_le_master_multilink = 0; // Master multi-link

#if TCFG_WIFI_DETECT_ENABLE
const int config_btctler_le_afh_en = 0; // Master AFH
#else /* TCFG_WIFI_DETECT_ENABLE */
const int config_btctler_le_afh_en = 1; // Master AFH
#endif /* TCFG_WIFI_DETECT_ENABLE */

/* LE RAM Control */
const int config_btctler_le_hw_nums           = 1;
const int config_btctler_le_rx_nums           = 8;
const int config_btctler_le_acl_packet_length = 27;
const int config_btctler_le_acl_total_nums    = 5;

/* ---------------------------------- */
/* ---------------------------------- DEF_BLE_WL_MIC_1T1_RX */
#elif (TCFG_BLE_DEMO_SELECT == DEF_BLE_WL_MIC_1T1_RX)

const int config_btctler_le_roles         = (LE_ADV | LE_SCAN | LE_INIT | LE_SLAVE | LE_MASTER);
const uint64_t config_btctler_le_features = LE_CORE_V50_FEATURES;
u32 config_vendor_le_bb = VENDOR_BB_PIS_EN |
                          VENDOR_BB_MD_CLOSE |
                          VENDOR_BB_PIS_HB |
                          VENDOR_BB_PIS_TX_PAYLOAD_LEN(55) |
                          VENDOR_BB_RX_PAYLOAD_LEN(TCFG_SERVER_RX_PAYLOAD_LEN) |
                          0;
const int config_btctler_le_master_multilink = 0; // Master multi-link
const int config_btctler_le_afh_en           = 0; // Master AFH
/* LE RAM Control */
const int config_btctler_le_hw_nums           = 4;
const int config_btctler_le_rx_nums           = 10;
const int config_btctler_le_acl_packet_length = 27;
const int config_btctler_le_acl_total_nums    = 2;

/* ---------------------------------- */
/* ---------------------------------- DEF_BLE_WL_MIC_1T1_TX */
#elif (TCFG_BLE_DEMO_SELECT == DEF_BLE_WL_MIC_1T1_TX)

const int config_btctler_le_roles         = (LE_ADV | LE_SCAN | LE_SLAVE | LE_INIT | LE_MASTER);
const uint64_t config_btctler_le_features = LE_CORE_V50_FEATURES;
u32 config_vendor_le_bb = VENDOR_BB_PIS_EN |
                          VENDOR_BB_MD_CLOSE |
                          VENDOR_BB_PIS_HB_M |
                          VENDOR_BB_ADV_PDU_INT(2) |
                          VENDOR_BB_RX_PAYLOAD_LEN(TCFG_SERVER_RX_PAYLOAD_LEN) |
                          VENDOR_BB_PIS_TX_PAYLOAD_LEN(107) |
                          0;
const int config_btctler_le_master_multilink = 0; // Master multi-link
const int config_btctler_le_afh_en           = 0; // Master AFH
/* LE RAM Control */
const int config_btctler_le_hw_nums           = 5;
const int config_btctler_le_rx_nums           = 5;
const int config_btctler_le_acl_packet_length = 251;
const int config_btctler_le_acl_total_nums    = 5;// 2;

/* ---------------------------------- */
/* ---------------------------------- DEF_BLE_WL_MIC_1TN_RX */
#elif (TCFG_BLE_DEMO_SELECT == DEF_BLE_WL_MIC_1TN_RX)

const int config_btctler_le_roles         = (LE_ADV | LE_SCAN | LE_SLAVE | LE_INIT | LE_MASTER);
const uint64_t config_btctler_le_features = LE_CORE_V50_FEATURES;
u32 config_vendor_le_bb = VENDOR_BB_PIS_EN |
                          VENDOR_BB_MD_CLOSE |
                          VENDOR_BB_PIS_HB |
                          VENDOR_BB_PIS_HB_R |
                          VENDOR_BB_PIS_TX_PAYLOAD_LEN(55) |
                          VENDOR_BB_RX_PAYLOAD_LEN(55) |
                          0;
const int config_btctler_le_master_multilink = 0; // Master multi-link
const int config_btctler_le_afh_en           = 0; // Master AFH
/* LE RAM Control */
const int config_btctler_le_hw_nums           = 4;
const int config_btctler_le_rx_nums           = 10;
const int config_btctler_le_acl_packet_length = 27;
const int config_btctler_le_acl_total_nums    = 2;

/* ---------------------------------- */
/* ---------------------------------- DEF_BLE_WL_MIC_1TN_TX */
#elif (TCFG_BLE_DEMO_SELECT == DEF_BLE_WL_MIC_1TN_TX)

const int config_btctler_le_roles         = (LE_ADV | LE_SCAN | LE_SLAVE | LE_INIT | LE_MASTER);
const uint64_t config_btctler_le_features = LE_CORE_V50_FEATURES;
u32 config_vendor_le_bb = VENDOR_BB_PIS_EN |
                          VENDOR_BB_MD_CLOSE |
                          VENDOR_BB_PIS_HB_M |
                          VENDOR_BB_ADV_PDU_INT(2) |
                          VENDOR_BB_RX_PAYLOAD_LEN(TCFG_SERVER_RX_PAYLOAD_LEN) |
                          VENDOR_BB_PIS_TX_PAYLOAD_LEN(107) |
                          0;
const int config_btctler_le_master_multilink = 0; // Master multi-link
const int config_btctler_le_afh_en           = 0; // Master AFH
/* LE RAM Control */
const int config_btctler_le_hw_nums           = 4;
const int config_btctler_le_rx_nums           = 5;
const int config_btctler_le_acl_packet_length = 251;
const int config_btctler_le_acl_total_nums    = 5;// 2;

#else /* TCFG_BLE_DEMO_SELECT == DEF_BLE_DEMO_MULTI */

const int config_btctler_le_roles             = (LE_ADV | LE_SLAVE);
const uint64_t config_btctler_le_features     = LE_ENCRYPTION;
u32 config_vendor_le_bb                 = 0;
const int config_btctler_le_master_multilink  = 0; // Master multi-link
const int config_btctler_le_afh_en            = 0; // Master AFH
/* LE RAM Control */
const int config_btctler_le_hw_nums           = 1;
const int config_btctler_le_rx_nums           = 5;
const int config_btctler_le_acl_packet_length = 27;
const int config_btctler_le_acl_total_nums    = 5;

#endif /* TCFG_BLE_DEMO_SELECT == DEF_BLE_DEMO_MULTI */

#else /* TCFG_USER_BLE_ENABLE */

const int config_btctler_le_roles             = 0;
const uint64_t config_btctler_le_features     = 0;
u32 config_vendor_le_bb                 = 0;
const int config_btctler_le_master_multilink  = 0; // Master multi-link
const int config_btctler_le_afh_en            = 0; // Master AFH
/* LE RAM Control */
const int config_btctler_le_hw_nums           = 0;
const int config_btctler_le_rx_nums           = 0;
const int config_btctler_le_acl_packet_length = 0;
const int config_btctler_le_acl_total_nums    = 0;

#endif /* TCFG_USER_BLE_ENABLE */

const int CONFIG_A2DP_DELAY_TIME            = 200;

const int config_btctler_le_slave_conn_update_winden = 2500;//range:100 to 2500

/*-----------------------------------------------------------*/
/**
 * @brief Bluetooth Analog setting
 */
/*-----------------------------------------------------------*/
#if ((!TCFG_USER_BT_CLASSIC_ENABLE) && TCFG_USER_BLE_ENABLE)
const int config_btctler_single_carrier_en = 1;   ////单模ble才设置
#else
const int config_btctler_single_carrier_en = 1;		//无线麦需要设置1，否则在打开双模的时候，无法使用测试盒
#endif

const int sniff_support_reset_anchor_point = 0;   //sniff状态下是否支持reset到最近一次通信点，用于HID

const int sniff_long_interval = (500 / 0.625);    //sniff状态下进入long interval的通信间隔(ms)

const int config_rf_oob = TCFG_RF_OOB;

/**
 * @brief Log (Verbose/Info/Debug/Warn/Error)
 */
/*-----------------------------------------------------------*/
//RF part
const char log_tag_const_v_Analog AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_i_Analog AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_w_Analog AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_d_Analog AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_e_Analog AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);

const char log_tag_const_v_RF AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_i_RF AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_d_RF AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_w_RF AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_e_RF AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);

//Classic part
const char log_tag_const_v_HCI_LMP AT(.LOG_TAG_CONST)  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_i_HCI_LMP AT(.LOG_TAG_CONST)  = CONFIG_DEBUG_LIB(1);
const char log_tag_const_d_HCI_LMP AT(.LOG_TAG_CONST)  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_w_HCI_LMP AT(.LOG_TAG_CONST)  = CONFIG_DEBUG_LIB(1);
const char log_tag_const_e_HCI_LMP AT(.LOG_TAG_CONST)  = CONFIG_DEBUG_LIB(1);

const char log_tag_const_v_LMP AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_i_LMP AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(1);
const char log_tag_const_d_LMP AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(1);
const char log_tag_const_w_LMP AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(1);
const char log_tag_const_e_LMP AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(1);

//LE part
const char log_tag_const_v_LE_BB AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_i_LE_BB AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_d_LE_BB AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_w_LE_BB AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_e_LE_BB AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);

const char log_tag_const_v_LE5_BB AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_i_LE5_BB AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_d_LE5_BB AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_w_LE5_BB AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_e_LE5_BB AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(1);

const char log_tag_const_v_HCI_LL AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_i_HCI_LL AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_d_HCI_LL AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_w_HCI_LL AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_e_HCI_LL AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);

const char log_tag_const_v_LL AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_i_LL AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_d_LL AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_w_LL AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_e_LL AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);

const char log_tag_const_v_LL_E AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_i_LL_E AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_d_LL_E AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_w_LL_E AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_e_LL_E AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);

const char log_tag_const_v_LL_M AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_i_LL_M AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_d_LL_M AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_w_LL_M AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_e_LL_M AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);

const char log_tag_const_v_LL_ADV AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_i_LL_ADV AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_d_LL_ADV AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_w_LL_ADV AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_e_LL_ADV AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);

const char log_tag_const_v_LL_SCAN AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_i_LL_SCAN AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_d_LL_SCAN AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_w_LL_SCAN AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_e_LL_SCAN AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);

const char log_tag_const_v_LL_INIT AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_i_LL_INIT AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_d_LL_INIT AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_w_LL_INIT AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_e_LL_INIT AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);

const char log_tag_const_v_LL_EXT_ADV AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_i_LL_EXT_ADV AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_d_LL_EXT_ADV AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_w_LL_EXT_ADV AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_e_LL_EXT_ADV AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);

const char log_tag_const_v_LL_EXT_SCAN AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_i_LL_EXT_SCAN AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_d_LL_EXT_SCAN AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_w_LL_EXT_SCAN AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_e_LL_EXT_SCAN AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);

const char log_tag_const_v_LL_EXT_INIT AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_i_LL_EXT_INIT AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_d_LL_EXT_INIT AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_w_LL_EXT_INIT AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_e_LL_EXT_INIT AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);

const char log_tag_const_v_LL_TWS_ADV AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_i_LL_TWS_ADV AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_d_LL_TWS_ADV AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(1);
const char log_tag_const_w_LL_TWS_ADV AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(1);
const char log_tag_const_e_LL_TWS_ADV AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(1);

const char log_tag_const_v_LL_TWS_SCAN AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_i_LL_TWS_SCAN AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_d_LL_TWS_SCAN AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(1);
const char log_tag_const_w_LL_TWS_SCAN AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(1);
const char log_tag_const_e_LL_TWS_SCAN AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(1);

const char log_tag_const_v_LL_S AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_i_LL_S AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_d_LL_S AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_w_LL_S AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_e_LL_S AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);

const char log_tag_const_v_LL_RL AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_i_LL_RL AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(1);
const char log_tag_const_d_LL_RL AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(1);
const char log_tag_const_w_LL_RL AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(1);
const char log_tag_const_e_LL_RL AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(1);

const char log_tag_const_v_LL_WL AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_i_LL_WL AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(1);
const char log_tag_const_d_LL_WL AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(1);
const char log_tag_const_w_LL_WL AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(1);
const char log_tag_const_e_LL_WL AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(1);

const char log_tag_const_v_AES AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_i_AES AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_d_AES AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(1);
const char log_tag_const_w_AES AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(1);
const char log_tag_const_e_AES AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(1);

const char log_tag_const_v_LL_PADV AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_i_LL_PADV AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_d_LL_PADV AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_w_LL_PADV AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_e_LL_PADV AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);

const char log_tag_const_v_LL_DX AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_i_LL_DX AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_d_LL_DX AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(1);
const char log_tag_const_w_LL_DX AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(1);
const char log_tag_const_e_LL_DX AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(1);

const char log_tag_const_v_LL_PHY AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_i_LL_PHY AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_d_LL_PHY AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(1);
const char log_tag_const_w_LL_PHY AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(1);
const char log_tag_const_e_LL_PHY AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(1);

const char log_tag_const_v_LL_AFH AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_i_LL_AFH AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_d_LL_AFH AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_w_LL_AFH AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_e_LL_AFH AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);

//HCI part
const char log_tag_const_v_Thread AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_i_Thread AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_d_Thread AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(1);
const char log_tag_const_w_Thread AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(1);
const char log_tag_const_e_Thread AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(1);

const char log_tag_const_v_HCI_STD AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_i_HCI_STD AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_d_HCI_STD AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_w_HCI_STD AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(1);
const char log_tag_const_e_HCI_STD AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(1);

const char log_tag_const_v_HCI_LL5 AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_i_HCI_LL5 AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_d_HCI_LL5 AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_w_HCI_LL5 AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_e_HCI_LL5 AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);

const char log_tag_const_v_BL AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_i_BL AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_d_BL AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_w_BL AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(1);
const char log_tag_const_e_BL AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(1);
const char log_tag_const_c_BL AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(1);


const char log_tag_const_v_TWS_LE AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(1);
const char log_tag_const_i_TWS_LE AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_d_TWS_LE AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(1);
const char log_tag_const_w_TWS_LE AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_e_TWS_LE AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_c_TWS_LE AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(1);


const char log_tag_const_v_TWS_LMP AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(1);
const char log_tag_const_i_TWS_LMP AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(1);
const char log_tag_const_d_TWS_LMP AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_w_TWS_LMP AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_e_TWS_LMP AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);


