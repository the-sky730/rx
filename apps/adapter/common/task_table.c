#include "system/includes.h"
#include "app_config.h"
#include "adapter_uart_demo.h"

/*任务列表, 注意:stack_size设置为32*n*/
const struct task_info task_info_table[] = {
    {"app_core",            1,  0,   896,  768  },
    {"sys_event",           6,  0,   256,   0    },
    {"btctrler",            5,  0,   512,   384  },
    {"btencry",             1,  0,   512,   128  },
#if TCFG_USER_TWS_ENABLE
    {"tws",                 5,  0,   512,   128  },
#endif
#if (TCFG_USER_TWS_ENABLE || TCFG_USER_BLE_ENABLE || TCFG_USER_EMITTER_ENABLE)
    {"btstack",             4,  0,   768,   256 + 512  },
#else
    {"btstack",             4,  0,   512,   256  },
#endif
#if TCFG_DEC2TWS_TASK_ENABLE
    {"local_dec",           3,  0,   768,   128  },
#endif
#if ((TCFG_USER_TWS_ENABLE && TCFG_MIC_EFFECT_ENABLE)||(SOUNDCARD_ENABLE))
    {"audio_dec",           3,  0,   768 + 128 + 128,   128  },
#else
    {"audio_dec",           3,  0,   768 + 32,   128  },
#endif
#if (TCFG_ENC_JLA_ENABLE)
    {"audio_enc",           3,  0,   512,   128  },
#else
    {"audio_enc",           3,  0,   512,   128  },
#endif
#if TCFG_AUDIO_DEC_OUT_TASK
    {"audio_out",           2,  0,   384,   0},
#endif
    {"aec",					2,	0,   768,   0	},
    {"aec_dbg",				3,	0,   128,   128	},
    {"update",				1,	0,   512,   0		},
#if(USER_UART_UPDATE_ENABLE)
    {"uart_update",	        1,	0,   256,   128	},
#endif
    {"systimer",		    6,	0,   128,   0		},
    {"dev_mg",           	3,  0,   512,   512  },
#ifndef CONFIG_SOUNDBOX_FLASH_256K
    {"usb_msd",           	1,  0,   512,   128   },
    {"usb_audio",           5,  0,   256,   256  },
    {"plnk_dbg",            5,  0,   256,   256  },
    {"adc_linein",          2,  0,   768,   128  },
    //{"src_write",           1,  0,   768,   256 	},
    {"fm_task",             3,  0,   512,   128  },
#endif
    {"enc_write",           1,  0,   768,   0 	},
#if (RCSP_BTMATE_EN || RCSP_ADV_EN)
    {"rcsp_task",			4,	0,   768,   128	},
#endif
#if TCFG_SPI_LCD_ENABLE
    {"ui",           	    2,  0,   768,   256  },
#else
    {"ui",                  3,  0,   384 - 64,   128  },
#endif

#if(TCFG_CHARGE_BOX_ENABLE)
    {"chgbox_n",            6,  0,   512,   128  },
#endif
#if (SMART_BOX_EN)
    {"smartbox",            4,  0,   768,   128  },
#endif//SMART_BOX_EN
#if (GMA_EN)
    {"tm_gma",              3,  0,   768,   256   },
#endif
#if RCSP_FILE_OPT
    {"file_bs",				1,	0,   768,	  64  },
#endif
#if TCFG_KEY_TONE_EN
    {"key_tone",            5,  0,   256,   32},
#endif
#if TCFG_MIXER_CYCLIC_TASK_EN
    {"mix_out",             5,  0,   256,   0},
#endif

    {"mic_stream",          5,  0,   768,   128  },
#if(TCFG_HOST_AUDIO_ENABLE)
    {"uac_play",            6,  0,   768,   0    },
    {"uac_record",          6,  0,   768,   32   },
#endif
    {"demo_task1",          2,  1,   768,   32   },
#if ADAPTER_UART_DEMO
    {"uart_task",           2,  0,   256,   48   },
#endif
    {0, 0},
};


