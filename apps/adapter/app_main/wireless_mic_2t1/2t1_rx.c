#include "adapter_idev.h"
#include "adapter_odev.h"
#include "adapter_idev_bt.h"
#include "adapter_odev_dac.h"
#include "adapter_process.h"
#include "adapter_media.h"
#include "application/audio_dig_vol.h"
#include "le_client_demo.h"
#include "system/malloc.h"
#include "key_event_deal.h"
#include "asm/pwm_led.h"
#include "ui_manage.h"
#include "adapter_idev_usb.h"
#include "adapter_wireless_command.h"
#include "audio_config.h"
#include "tone_player.h"
#include "stream_sync.h"
#include "adapter_encoder.h"
#include "audio_sound_dac.h"
#include "ble/ll_config.h"
#include "adapter_uart_demo.h"


#if (APP_MAIN ==  APP_WIRELESS_MIC_2T1 && WIRELESS_ROLE_SEL == APP_WIRELESS_MASTER)

#define POWER_OFF_CNT       6
#define SLAVE_ENC_COMMAND_LEN		2


struct __expand_stream {
    struct audio_stream *stream;     // 音频流
    struct audio_stream_entry entry; // usb 输出音频入口
    int out_len;
    int process_len;

    struct __stream_sync *sync;
    struct channel_switch *channel_zoom;
    struct adapter_encoder *encode;
};

struct __expand_stream *expand_stream = NULL;


static struct adapter_media *wireless_mic_media = NULL;
u8 wireless_conn_status = 0;
static u8 cur_dac_gain = 0;

extern void adapter_wireless_enc_command_send(u8 command);
extern u32 adapter_usb_mic_get_output_total_size(void *priv);
extern u32 adapter_usb_mic_get_output_size(void *priv);
extern u8 get_charge_online_flag(void);
extern u8 app_common_device_event_deal(struct sys_event *event);

static const struct idev_usb_fmt usb_parm_list[] = {
    //HID
#if 0
    {
        .attr  = IDEV_USB_ATTR_HID,
        .value =
        {
            .hid = {
                .report_map_num = 1,
                .report_map[0] = {
                    .map = NULL,
                    .size = 0,
                }
            },
        },
    },
#endif
    //END
    {
        .attr = IDEV_USB_ATTR_END,
    },
};



//主机音频设置
static const u16 master_dvol_table[] = {
    0	, //0
    93	, //1
    111	, //2
    132	, //3
    158	, //4
    189	, //5
    226	, //6
    270	, //7
    323	, //8
    386	, //9
    462	, //10
    552	, //11
    660	, //12
    789	, //13
    943	, //14
    1127, //15
    1347, //16
    1610, //17
    1925, //18
    2301, //19
    2751, //20
    3288, //21
    3930, //22
    4698, //23
    5616, //24
    6713, //25
    8025, //26
    9592, //27
    11466,//28
    15200,//29
    16000,//30
    16384 //31
};

static const audio_dig_vol_param master_digital_vol_parm = {
    .vol_start = ARRAY_SIZE(master_dvol_table) - 1,
    .vol_max = ARRAY_SIZE(master_dvol_table) - 1,
#if (TCFG_AUDIO_DAC_CONNECT_MODE == DAC_OUTPUT_LR)
    .ch_total = 2,
#else
    .ch_total = 1,
#endif
    .fade_en = 1,
    .fade_points_step = 2,
    .fade_gain_step = 2,
    .vol_list = (void *)master_dvol_table,
};


static u16 wireless_mic_upstream_in_sr(void *priv)
{
    return WIRELESS_CODING_SAMPLERATE;
}
static u16 wireless_mic_upstream_out_sr(void *priv)
{
    return MIC_AUDIO_RATE;//USB mic接type C连接手机， 48000兼容性好， 所以这里用48000这个采样率
}


extern u32 adapter_usb_mic_get_output_total_size(void *priv);
extern u32 adapter_usb_mic_get_output_size(void *priv);


struct adapter_encoder_fmt master_audio_stream_parm = {
    .enc_type = ADAPTER_ENC_TYPE_UAC_MIC,
};



static void audio_stream_expand_resume(void *p)
{

}

static void audio_stream_expand_data_process_len(struct audio_stream_entry *entry, int len)
{

    struct __expand_stream *hdl = container_of(entry, struct __expand_stream, entry);
    hdl->out_len += len;
    /* printf("out len[%d]",hdl->out_len); */
}


extern struct __stream_sync *adapter_audio_stream_sync_open(struct adapter_sync_parm *parm);
static const struct adapter_sync_parm sync_param = {
    //其他关于sync的参数配置
    .always = 1,
#if (MIC_CHANNEL == 2)
    .ch_num = 2,//mic 是双声道
    .ibuf_len = (WIRELESS_DECODE_SAMPLERATE *WIRELESS_CODING_FRAME_LEN / 10000) * 4, //48K, 360*2
    .obuf_len = (WIRELESS_DECODE_SAMPLERATE *WIRELESS_CODING_FRAME_LEN / 10000) * 4 + 480, //inbuf基础上加余量， 32点
#else
    .ch_num = 1,//mic 是单声道
    .ibuf_len = (WIRELESS_DECODE_SAMPLERATE *WIRELESS_CODING_FRAME_LEN / 10000) * 2, //48K, 360*2
    .obuf_len = (WIRELESS_DECODE_SAMPLERATE *WIRELESS_CODING_FRAME_LEN / 10000) * 2 + 240, //inbuf基础上加余量， 32点
#endif
    .begin_per = 40,
    .top_per = 60,
    .bottom_per = 30,
    .inc_step = 10,
    .dec_step = 10,
    .max_step = 100,
    .get_in_sr = wireless_mic_upstream_in_sr,
    .get_out_sr = wireless_mic_upstream_out_sr,
    .get_total = adapter_usb_mic_get_output_total_size,
    .get_size = adapter_usb_mic_get_output_size,
    .data_handle = NULL,//如果要关注处理前的数据可以在这里注册回调
};


static void audio_stream_expand_open(void)
{
    struct __expand_stream *hdl = zalloc(sizeof(struct __expand_stream));
    ASSERT(hdl);

    hdl->entry.data_process_len = audio_stream_expand_data_process_len;

    struct audio_stream_entry *entries[15] = {NULL};
    u8 entry_cnt = 0;

    entries[entry_cnt++] = &hdl->entry;

    //声道转换
    u8 channel_out_type = (MIC_CHANNEL == 2 ? AUDIO_CH_LR : AUDIO_CH_DIFF);
    hdl->channel_zoom = channel_switch_open(channel_out_type, 0);
    if (hdl->channel_zoom) {
        entries[entry_cnt++] = &hdl->channel_zoom->entry;
    }

    //src
    hdl->sync = adapter_audio_stream_sync_open(&sync_param);
    if (hdl->sync) {
        entries[entry_cnt++] = &hdl->sync->entry;
    }

    struct adapter_media_parm media_parm = {0};
    media_parm.sample_rate = MIC_AUDIO_RATE;
    media_parm.ch_num = MIC_CHANNEL;
    hdl->encode = adapter_encoder_open(&master_audio_stream_parm, &media_parm);
    if (hdl->encode) {
        entries[entry_cnt++] = adapter_encoder_get_stream_entry(hdl->encode);
    }

    hdl->stream = audio_stream_open(hdl, audio_stream_expand_resume);
    audio_stream_add_list(hdl->stream, entries, entry_cnt);

    expand_stream = hdl;
}

static void audio_stream_expand_close(void)
{
    if (expand_stream) {
        //删除数据流节点
        if (expand_stream->channel_zoom) {
            channel_switch_close(&expand_stream->channel_zoom);
        }

        if (expand_stream->sync) {
            stream_sync_close(&expand_stream->sync);
        }

        if (expand_stream->encode) {
            audio_stream_del_entry(adapter_encoder_get_stream_entry(expand_stream->encode));
            adapter_encoder_close(expand_stream->encode);
        }

        //释放数据流
        if (expand_stream->stream) {
            audio_stream_close(expand_stream->stream);
        }

        free(expand_stream);
        expand_stream = NULL;
    }
}

static void audio_stream_expand_run(struct audio_data_frame *in)
{
    if (expand_stream == NULL) {
        return ;
    }
    //putchar('r');
    struct audio_data_frame frame = {0};
    frame.channel = in->channel;
    frame.sample_rate = WIRELESS_DECODE_SAMPLERATE;
    frame.data_len = in->data_len;
    frame.data = in->data;
    expand_stream->out_len = 0;
    expand_stream->process_len = in->data_len;

    while (1) {
        audio_stream_run(&expand_stream->entry, &frame);
        if (expand_stream->out_len >= expand_stream->process_len) {
            break;
        }
        frame.data = (s16 *)((u8 *)in->data + expand_stream->out_len);
        frame.data_len = in->data_len - expand_stream->out_len;
    }
}

#if (WIRELESS_STEREO_OUTPUT_DIFF)
static void pcm_dual_to_spec_diff(void *data, u16 len)
{
    s16 *inbuf = data;
    s16 x;
    len >>= 2;
    while (len--) {
        /* x = inbuf[1]; */
        /* x = (x == -32768)?32767:-x; */
        /* inbuf[1] = x; */
        inbuf[1] = (inbuf[1] == -32768) ? 32767 : -inbuf[1];
        inbuf += 2;
    }
}
#endif

static int data_pro_handler(struct audio_stream_entry *entry,  struct audio_data_frame *in)
{
    if (in->data_len == 0) {
        return 0;
    }
#if (WIRELESS_MIC_RX_OUTPUT_SEL == WIRELESS_MIC_RX_OUTPUT_USB_MIC)
    audio_stream_expand_run(in);
#endif

#if (WIRELESS_STEREO_OUTPUT_DIFF)
    pcm_dual_to_spec_diff(in->data, in->data_len);
#endif

    return 0;
}

static const struct adapter_stream_fmt master_audio_stream_list[] = {


#if (WIRELESS_MIC_RX_OUTPUT_SEL == WIRELESS_MIC_RX_OUTPUT_USB_MIC)
    //数字音量节点
    {
        .attr  = ADAPTER_STREAM_ATTR_DVOL,
        .value = {
            .digital_vol = {
                .data_handle = NULL,//如果要关注处理前的数据可以在这里注册回调
                .volume = &master_digital_vol_parm,
            },
        },
    },
#endif

#if 0//(TCFG_AUDIO_MONO_LR_DIFF_CHANEL_SWITCH_ENABLE)
    {
        .attr = ADAPTER_STREAM_ATTR_CH_SW,
        .value = {
            .ch_sw = {
                .data_handle = data_pro_handler,//如果要关注处理前的数据可以在这里注册回调
                .out_ch_type = AUDIO_CH_DUAL_DIFF,
                .buf_len = 0,
            },
        },
    },
#endif

    //输出到USB 同时也会输出到dac，dac输出前将数据拷贝给usb mic
    {
        .attr  = ADAPTER_STREAM_ATTR_DAC,
        .value = {
#if NEW_AUDIO_W_MIC
            .dac1 = {
#if (TCFG_AUDIO_MONO_LR_DIFF_CHANEL_SWITCH_ENABLE)
                .data_handle = NULL,//如果要关注处理前的数据可以在这里注册回调
#else
                .data_handle = data_pro_handler,//如果要关注处理前的数据可以在这里注册回调
#endif
                .attr = {
                    .write_mode = WRITE_MODE_BLOCK,
                    .sample_rate = WIRELESS_DECODE_SAMPLERATE,
#if (WIRELESS_CODING_FRAME_LEN <= 50)
                    .delay_ms = 12,
                    .protect_time = 0,
                    .start_delay = 6,
                    .underrun_time = 1,
#elif (WIRELESS_CODING_FRAME_LEN == 75)
                    .delay_ms = 13,
                    .protect_time = 0,
                    .start_delay = 8,
                    .underrun_time = 2,
#elif (WIRELESS_CODING_FRAME_LEN == 100)
                    .delay_ms = 20,
                    .protect_time = 0,
                    .start_delay = 8,
                    .underrun_time = 2,
#endif
                },
            }

#else
            .dac = {
#if (TCFG_AUDIO_MONO_LR_DIFF_CHANEL_SWITCH_ENABLE)
                .data_handle = NULL,//如果要关注处理前的数据可以在这里注册回调
#else
                .data_handle = data_pro_handler,//如果要关注处理前的数据可以在这里注册回调
#endif
                //其他关于dac的参数配置
                .get_delay_time = NULL,
                .attr = {
                    .write_mode = WRITE_MODE_BLOCK,
                    .delay_time = 12,//10,//dac延时设置
                    .protect_time = 0,
                    .start_delay = 6,
                    .underrun_time = 1,
                },
            },

#endif
        },
    },
};
static int master_audio_command_parse(void *priv, u8 *data)
{
    /*!< 这里回调是音频传输过程附带过来的命令数据 */

#if WIRELESS_MIC_DAC_ONLINE_CONFIG
    if ((wireless_conn_status == 1) || (wireless_conn_status == 2)) { //该功能限制了只连接一个TX有效， 而且该功能只适合用于调试阶段, 方便调试， 量产阶段不要开
        if (data[0] & 0xA0) { //DAC设置校验
            if (cur_dac_gain != (data[0] & 0x0F)) {
                cur_dac_gain = data[0] & 0x0F;
                adapter_decoder_ioctrl(wireless_mic_media->downdecode, ADAPTER_DEC_IOCTRL_CMD_DAC_GAIN_SET, 1, cur_dac_gain);
            }
        }
    }
#endif
    //2t1使能立体声输入功能，可以在此获取左右声道发过来的数据:channel 0->左 1->右
    u8 channel = (u8)(priv);
    if (data[1]) {
#if ADAPTER_UART_DEMO
        uart_duplex_send_data(&data[1], 1);
#endif
    }
    //test code
    /* if (channel == 0) {//左声道 */
    /* put_buf(data,SLAVE_ENC_COMMAND_LEN);	 */
    /* } else if (channel == 1) { //右声道 */
    /* put_buf(data,SLAVE_ENC_COMMAND_LEN);	 */
    /* } */
    return SLAVE_ENC_COMMAND_LEN;
}
static struct adapter_decoder_fmt master_audio_stream = {
    .dec_type = ADAPTER_DEC_TYPE_DUAL_JLA,
    .list	  = master_audio_stream_list,
#if (TCFG_AUDIO_DAC_CONNECT_MODE == DAC_OUTPUT_LR && (WIRELESS_TX_MIC_STEREO_OUTPUT == 0))
    .dec_output_type = AUDIO_CH_LR,
#else
    .dec_output_type = AUDIO_CH_DIFF,
#endif
    .list_num = sizeof(master_audio_stream_list) / sizeof(master_audio_stream_list[0]),
    .cmd_parse_cbk = master_audio_command_parse,
};
static const struct adapter_media_fmt master_media_list[] = {
    /* [0] = { */
    /* .upstream =	NULL, */
    /* .downstream = &master_audio_stream, */
    /* }, */
    [0] = {
        .upstream =	NULL,
        .downstream = &master_audio_stream,
    },
    /* [1] = { */
    /*     .upstream =	&master_audio_stream, */
    /*     .downstream = NULL, */
    /* }, */
};
static const struct adapter_media_config master_media_config = {
    .list = master_media_list,
    .list_num = sizeof(master_media_list) / sizeof(master_media_list[0]),
};


//bt idev config
struct _odev_bt_parm odev_bt_parm_list = {
    .mode = BIT(ODEV_BLE),
};

//bt odev config
#if !WIRELESS_TOOL_BLE_NAME_EN
static u8 dongle_remoter_name1[] = "W_MIC_01";
static client_match_cfg_t match_dev01 = {
    .create_conn_mode = BIT(CLI_CREAT_BY_NAME),
    .compare_data_len = sizeof(dongle_remoter_name1) - 1, //去结束符
    .compare_data = dongle_remoter_name1,
    .bonding_flag = 0,
};
#else
static u8 *match_name;
client_match_cfg_t match_dev01 = {0};
#endif


static client_conn_cfg_t ble_conn_config = {
    .match_dev_cfg[0] = &match_dev01,      //匹配指定的名字
    .match_dev_cfg[1] = NULL,
    .match_dev_cfg[2] = NULL,
    .search_uuid_cnt = 0,
    .security_en = 0,       //加密配对
};

struct _idev_bt_parm idev_bt_parm_list = {
    .mode = BIT(IDEV_BLE), //only support BLE

    //for BLE config
    .ble_parm.cfg_t = &ble_conn_config,
};



u8 flag_mute_exdac = 0;
static int adapter_key_event_handler(struct sys_event *event)
{
    int ret = 0;
    struct key_event *key = &event->u.key;

    u16 key_event = event->u.key.event;
    static u8 channel_sw = 1;

    static u8 key_poweroff_cnt = 0;
    static u8 flag_poweroff = 0;
    static u8 test_buf[10] = {0};

    printf("key_event:%d %d %d\n", key_event, key->value, key->event);
    switch (key_event) {
    case KEY_WIRELESS_2t1_RX_SEND_DATA:
        printf("send to L");
        memset(test_buf, 0xAA, sizeof(test_buf));
        put_buf(test_buf, sizeof(test_buf));
        wireless_mic_client_send_data(0, test_buf, sizeof(test_buf));
        os_time_dly(10);
        printf("send to R");
        memset(test_buf, 0x55, sizeof(test_buf));
        put_buf(test_buf, sizeof(test_buf));
        wireless_mic_client_send_data(1, test_buf, sizeof(test_buf));
        break;

#if WIRELESS_TX_MIC_STEREO_OUTPUT
    //两发一收 立体声输出 使用按键切换真立体声和假立体声
    case KEY_WIRELESS_MIC_CH_SW:
        channel_sw = !channel_sw;
        printf("channle switch = %d", channel_sw);
        int ch = 0 << 30;
        adapter_decoder_ioctrl(wireless_mic_media->downdecode, ADAPTER_DEC_IOCTRL_CMD_MIXER_CHANNEL_SWITCH | ch, 1, (int *)channel_sw);
        ch = 1 << 30;
        adapter_decoder_ioctrl(wireless_mic_media->downdecode, ADAPTER_DEC_IOCTRL_CMD_MIXER_CHANNEL_SWITCH | ch, 1, (int *)channel_sw);
        break;
#endif

    case KEY_SW_SAMETIME_OUTPUT:
        //开关同时输出到dac
        flag_mute_exdac = !flag_mute_exdac;
        printf("flag_mute_exdac = %d", flag_mute_exdac);
        break;
    case  KEY_POWEROFF:
        printf("KEY POWEROFF\n");
        key_poweroff_cnt = 0;
        flag_poweroff = 1;
        break;
    case  KEY_POWEROFF_HOLD:
        printf("KEY POWEROFF_HOLD\n");
        if (flag_poweroff) {
            if (++key_poweroff_cnt >= POWER_OFF_CNT) {
                key_poweroff_cnt = 0;
                ret = 1;
            }
        }

        break;

    case  KEY_NULL:
        break;
    }
    return ret;
}


extern u32 config_vendor_le_bb;
static int event_handle_callback(struct sys_event *event)
{
    //处理用户关注的事件
    int ret = 0;
    switch (event->type) {
    case SYS_KEY_EVENT:
        ret = adapter_key_event_handler(event);
        break;
    case SYS_DEVICE_EVENT:
        app_common_device_event_deal(event);
        switch ((u32)event->arg) {
        case DEVICE_EVENT_FROM_ADAPTER:
            switch (event->u.dev.event) {
            case ADAPTER_EVENT_CONNECT :
                if (event->u.dev.value == 1) {
                    printf("ADAPTER_EVENT_CONNECT_FIRST\n");
                    wireless_conn_status |= BIT(0);
                } else {
                    printf("ADAPTER_EVENT_CONNECT_SECOND\n");
                    wireless_conn_status |= BIT(1);
                }
                ui_update_status(STATUS_BT_CONN);
                sys_auto_shut_down_disable();
                break;
            case ADAPTER_EVENT_DISCONN :
                // if (event->u.dev.value == 1) {
                //     printf("ADAPTER_EVENT_DISCONN_FIRST\n");
                //     wireless_conn_status &= ~BIT(1);
                // } else {
                //     printf("ADAPTER_EVENT_DISCONN_SECOND\n");
                //     wireless_conn_status &= ~BIT(0);
                // }

                if(event->u.dev.value == 1){
                    printf("ADAPTER_EVENT_DISCONN_FIRST\n");
                    wireless_conn_status &= ~BIT(0);
                }else if(event->u.dev.value == 2){
                    printf("ADAPTER_EVENT_DISCONN_SECOND\n");
                    wireless_conn_status &= ~BIT(1);
                }else{
                    wireless_conn_status = 0;
                }

                ui_update_status(STATUS_BT_DISCONN);
                if (!wireless_conn_status) {
                    sys_auto_shut_down_enable();
                }
                break;
            case ADAPTER_EVENT_IDEV_MEDIA_CLOSE :
            case ADAPTER_EVENT_ODEV_MEDIA_CLOSE :
                break;
            case ADAPTER_EVENT_POWEROFF:
                ret = 1;
                break;
#if TCFG_2T1_RX_PRODUCT_TEST_EN
            case ADAPTER_EVENT_SWITCH_RX_TEST:
                extern void ll_hci_reset(void);
                ll_hci_reset();
                config_vendor_le_bb = VENDOR_BB_PIS_EN |
                                      VENDOR_BB_MD_CLOSE |
                                      /* VENDOR_BB_PIS_HB | */
                                      VENDOR_BB_PIS_TX_PAYLOAD_LEN(55) |
                                      VENDOR_BB_RX_PAYLOAD_LEN(TCFG_SERVER_RX_PAYLOAD_LEN) |
                                      0;
                extern void wlm_2t1_test_rx_init(void);
                wlm_2t1_test_rx_init();

                break;
#endif
            }
        }
        break;
    default:
        break;

    }
    return ret;
}
u8 flag_specific_sacn = 0;
void wireless_cfg_tool_read_name(void)
{
    u8 ble_name_len;
    extern const char *bt_get_local_name();
    match_name = (u8 *)(bt_get_local_name());
    if (0 == memcmp(match_name, SPECIFIC_STRING, strlen(SPECIFIC_STRING))) {
        match_name = &match_name[strlen(SPECIFIC_STRING)];
        printf("specific scan%s", match_name);
        flag_specific_sacn = 1;
    }

    ble_name_len = strlen(match_name);
    /* memcpy(match_dev01.compare_data, match_name, ble_name_len); */

    match_dev01.create_conn_mode = BIT(CLI_CREAT_BY_NAME);
    match_dev01.compare_data_len = ble_name_len;
    match_dev01.compare_data = match_name;
    match_dev01.bonding_flag = 0;
    printf("%s", match_dev01.compare_data);

}
void app_main_run(void)
{
#if ADAPTER_UART_DEMO
    adapter_uart_demo_init();
#endif
    ui_update_status(STATUS_POWERON);
    //dac_init
    if (audio_dac_init_status() == 0) {
        app_audio_output_init();
    }
#if WIRELESS_TOOL_BLE_NAME_EN
    wireless_cfg_tool_read_name();
#endif

    while (1) {
        //初始化
        struct idev *idev = adapter_idev_open(ADAPTER_IDEV_BT, (void *)&idev_bt_parm_list);
#if (WIRELESS_MIC_RX_OUTPUT_SEL == WIRELESS_MIC_RX_OUTPUT_USB_MIC)
        struct odev *odev = adapter_odev_open(ADAPTER_ODEV_USB, NULL);
#else
        struct idev *odev = adapter_odev_open(ADAPTER_ODEV_DAC, NULL);
#endif
        struct adapter_media *media = adapter_media_open((struct adapter_media_config *)&master_media_config);
#if WIRELESS_TX_MIC_STEREO_OUTPUT
        //默认输出选择1:真立体 0:假立体
        media->downstream_parm.mixer_output_select = 1;
#endif
        printf("wireless_mic_2t1_rx ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");

#if (WIRELESS_MIC_RX_OUTPUT_SEL == WIRELESS_MIC_RX_OUTPUT_USB_MIC)
        audio_stream_expand_open();
#endif

        ASSERT(idev);
        ASSERT(odev);
        ASSERT(media);
        struct adapter_pro *pro = adapter_process_open(idev, odev, media, event_handle_callback);//event_handle_callback 用户想拦截处理的事件

        ASSERT(pro, "adapter_process_open fail!!\n");

        wireless_mic_media = media;

        //执行(包括事件解析、事件执行、媒体启动/停止, HID等事件转发)
        adapter_process_run(pro);

        wireless_mic_media = NULL;

        //退出/关闭
        adapter_process_close(&pro);
        adapter_media_close(&media);
        adapter_idev_close(idev);
        adapter_odev_close(odev);


#if (WIRELESS_MIC_RX_OUTPUT_SEL == WIRELESS_MIC_RX_OUTPUT_USB_MIC)
        audio_stream_expand_close();
#endif

        ui_update_status(STATUS_POWEROFF);
        ///run idle off poweroff
        printf("enter poweroff !!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
        if (get_charge_online_flag()) {
            printf("charge_online,cpu reset");
            cpu_reset();
        } else {
            power_set_soft_poweroff();
        }
    }
}


#endif

