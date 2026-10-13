#include "key_event_deal.h"
#include "key_driver.h"
#include "app_config.h"
#include "app_task.h"

#ifdef CONFIG_BOARD_WIRELESS_MIC_2T1_RX
const u16 key_rdec_table[KEY_RDEC_NUM_MAX][KEY_EVENT_MAX] = {
    [0] = {
        KEY_NULL,		KEY_NULL,		KEY_NULL,		KEY_NULL,		KEY_NULL,		KEY_NULL
    },
    [1] = {
        KEY_NULL,		KEY_NULL,		KEY_NULL,		KEY_NULL,		KEY_NULL,		KEY_NULL
    },
    [2] = {
        KEY_NULL,		KEY_NULL,		KEY_NULL,		KEY_NULL,		KEY_NULL,		KEY_NULL
    },
};
#endif
