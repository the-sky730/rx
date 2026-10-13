#ifndef __ADAPTER_AUDIO_STREAM_H___
#define __ADAPTER_AUDIO_STREAM_H___

#include "app_config.h"
#include "generic/typedef.h"
#include "media/includes.h"
#include "stream_entry.h"
#include "application/audio_dig_vol.h"
//#include "application/audio_noisegate.h"
#include "media/audio_noisegate.h"
#include "media/audio_voice_changer.h"
#include "media/audio_eq_drc_apply.h"
#include "stream_src.h"
#include "asm/dac.h"
#include "adapter_media.h"
#include "stream_HighSampleRateMicSystem.h"
#ifndef CONFIG_EFFECT_CORE_V2_ENABLE
#include "application/reverb_api.h"
#else
#include "media/reverb_api.h"
#endif
#include "adapter_encoder.h"
#include "application/audio_energy_detect.h"
#include "stream_eq_dccs.h"
#include "audio_sound_dac.h"
#include "channel_switch.h"
#include "adapter_stream_demo.h"
#include "stream_llns.h"
#include "media/audio_howling.h"

enum adapter_stream_attr {
    ADAPTER_STREAM_ATTR_SYNC = 0x0,						/*!< 同步数据流节点属性 */
    ADAPTER_STREAM_ATTR_ENTRY_DEBUG,					/*!< 调试数据流节点属性 */
    ADAPTER_STREAM_ATTR_ENCODE,							/*!< 编码数据流节点属性 */
    ADAPTER_STREAM_ATTR_SRC,							/*!< 变采样数据流节点属性 */
    ADAPTER_STREAM_ATTR_DVOL,							/*!< 数字音量数据流节点属性 */
    ADAPTER_STREAM_ATTR_DAC,							/*!< DAC数据流节点属性 */
    ADAPTER_STREAM_ATTR_NOISEGATE,						/*!< 噪声门限数据流节点属性 */
    ADAPTER_STREAM_ATTR_DENOISE,						/*!< 降噪数据流节点属性 */
    ADAPTER_STREAM_ATTR_MICEQ,							/*!< mic 上行EQ数据流节点属性 */
    ADAPTER_STREAM_ATTR_MUSICEQ,						/*!< 音乐下行EQ数据流节点属性 */
    ADAPTER_STREAM_ATTR_DCCS,							/*!< dccs数据流节点属性 */
    ADAPTER_STREAM_ATTR_ECHO,							/*!< 混响数据流节点属性 */
    ADAPTER_STREAM_ATTR_PLATE_REVERB,					/*!< 混响数据流节点属性 */
    ADAPTER_STREAM_ATTR_AUTO_MUTE,						/*!< 自动mute数据流节点属性 */
    ADAPTER_STREAM_ATTR_STREAM_DEMO,					/*!< stream_demo流节点属性 */
    ADAPTER_STREAM_ATTR_LLNS,							/*!< 低延时降噪节点属性 */
    ADAPTER_STREAM_ATTR_HOWLING,						/*!< 啸叫抑制数据流节点属性 */
    ADAPTER_STREAM_ATTR_CH_SW,						    /*!< 声道转换数据流节点属性 */
    ADAPTER_STREAM_ATTR_HOWLINGSUPPRESS,			    /*!< 陷波啸叫抑制数据流节点属性 */
    ADAPTER_STREAM_ATTR_VOICE_CHANGER,			        /*!< 变声数据流节点属性 */
    ADAPTER_STREAM_ATTR_DRC,                            /*!< drc数据流节点属性 */
};

struct adapter_encode_stream_parm {
    struct adapter_encoder_fmt *parm;
    int (*data_handle)(struct audio_stream_entry *entry,  struct audio_data_frame *in);/*!< 该数据流数据处理前数据回调,可以实现数据检测及做特殊数字处理 */
};
struct adapter_digital_vol_parm {
    audio_dig_vol_param *volume;	/*!< 数字音量参数，这里是结构体指针 */
    int (*data_handle)(struct audio_stream_entry *entry,  struct audio_data_frame *in);/*!< 该数据流数据处理前数据回调,可以实现数据检测及做特殊数字处理 */
};

struct adapter_src_parm {
    u16 target_sr;	/*!< 变采样的目标采样率 */
    u8 always;		/*!< 无论采样率是否一致都变采样使能 */
    int (*data_handle)(struct audio_stream_entry *entry,  struct audio_data_frame *in);/*!< 该数据流数据处理前数据回调,可以实现数据检测及做特殊数字处理 */
};


struct adapter_sync_parm {
    u8	ch_num;						/*!< 通道 */
    int ibuf_len;                   /*!< 外部分配同步输入buf大小 */
    int obuf_len;                   /*!< 外部分配同步输出buf大小 */
    int begin_per;					/*!< 起始百分比 */
    int top_per;					/*!< 最大百分比 */
    int bottom_per;					/*!< 最小百分比 */
    u8 inc_step;					/*!< 每次调整增加步伐 */
    u8 dec_step;					/*!< 每次调整减少步伐 */
    u8 max_step;					/*!< 最大调整步伐 */
    u8 always;						/*!< 无论采样率是否一致都变采样使能 */
    void *priv;						/*!< get_total,get_size私有句柄 */
    u16(*get_in_sr)(void *priv);	/*!< 获取输入采样率 */
    u16(*get_out_sr)(void *priv);	/*!< 获取输出采样率 */
    int (*get_total)(void *priv);	/*!< 获取buf总长 */
    int (*get_size)(void *priv);	/*!< 获取buf数据长度 */
    int (*data_handle)(struct audio_stream_entry *entry,  struct audio_data_frame *in);/*!< 该数据流数据处理前数据回调,可以实现数据检测及做特殊数字处理 */
};

struct adapter_debug_entry_parm {
    int (*data_handle)(struct audio_stream_entry *entry,  struct audio_data_frame *in);/*!< 该数据流数据处理前数据回调,可以实现数据检测及做特殊数字处理 */
    u8 is_end;
};

struct adapter_stream_demo_parm {
    int (*data_handle)(struct audio_stream_entry *entry,  struct audio_data_frame *in);/*!< 该数据流数据处理前数据回调,可以实现数据检测及做特殊数字处理 */
};

struct adapter_dac_parm {
    struct audio_dac_channel_attr attr;	/*!< dac 输出参数配置 */
    u8(*get_delay_time)(void);   /*!< 通过回调获取dac延时 */
    int (*data_handle)(struct audio_stream_entry *entry,  struct audio_data_frame *in);/*!< 该数据流数据处理前数据回调,可以实现数据检测及做特殊数字处理 */
};
struct adapter_dac1_parm {
    // struct audio_dac_channel_attr attr;	[>!< dac 输出参数配置 <]
    // u8(*get_delay_time)(void);   [>!< 通过回调获取dac延时 <]
    struct dac_param attr;
    int (*data_handle)(struct audio_stream_entry *entry,  struct audio_data_frame *in);/*!< 该数据流数据处理前数据回调,可以实现数据检测及做特殊数字处理 */
};
struct adapter_noisegate_parm {
    /* #ifndef CONFIG_EFFECT_CORE_V2_ENABLE */
    // NOISEGATE_PARM *parm;	[>!< 噪声门限参数 <]
    /* #else */
    NoiseGateParam *parm;  //参数
// #endif
    int (*data_handle)(struct audio_stream_entry *entry,  struct audio_data_frame *in);/*!< 该数据流数据处理前数据回调,可以实现数据检测及做特殊数字处理 */
};
struct adapter_HighSampleRateSingleMicSystem_parm {
    u16	sample_rate; /*!< 采样率 */
    u8	onoff; 		 /*!< 初始化状态设置， 开/关 */
    struct __stream_HighSampleRateSingleMicSystem_parm 	parm; /*!< 降噪参数 */
    int (*data_handle)(struct audio_stream_entry *entry,  struct audio_data_frame *in);/*!< 该数据流数据处理前数据回调,可以实现数据检测及做特殊数字处理 */
};
struct adapter_eq_parm {
    u16 samplerate;
    u8 ch_num;
    int (*data_handle)(struct audio_stream_entry *entry,  struct audio_data_frame *in);/*!< 该数据流数据处理前数据回调,可以实现数据检测及做特殊数字处理 */
};
struct adapter_dccs_parm {
    struct stream_eq_dccs_parm *parm;	/*!< eq dccs参数 */
    int (*data_handle)(struct audio_stream_entry *entry,  struct audio_data_frame *in);/*!< 该数据流数据处理前数据回调,可以实现数据检测及做特殊数字处理 */
};
struct adapter_echo_parm {
    ECHO_PARM_SET *parm;	/*!< echo 参数 */
    EF_REVERB_FIX_PARM *fix_parm;
    int (*data_handle)(struct audio_stream_entry *entry,  struct audio_data_frame *in);/*!< 该数据流数据处理前数据回调,可以实现数据检测及做特殊数字处理 */
};
struct adapter_plate_reverb_parm {
#ifdef CONFIG_EFFECT_CORE_V2_ENABLE
    Plate_reverb_parm *parm;	/*!< reverb 参数 */
#endif
    u16 samplerate;
    int (*data_handle)(struct audio_stream_entry *entry,  struct audio_data_frame *in);/*!< 该数据流数据处理前数据回调,可以实现数据检测及做特殊数字处理 */
};
struct adapter_auto_mute_parm {
    audio_energy_detect_param *parm;	/*!< auto_mute 参数 */
    int (*data_handle)(struct audio_stream_entry *entry,  struct audio_data_frame *in);/*!< 该数据流数据处理前数据回调,可以实现数据检测及做特殊数字处理 */
};

struct adapter_howling_parm {
    void *howl_para;
    u16 sample_rate;
    u8 channel;
    u8 mode;
    int (*data_handle)(struct audio_stream_entry *entry,  struct audio_data_frame *in);/*!< 该数据流数据处理前数据回调,可以实现数据检测及做特殊数字处理 */
};
struct adapter_llns_parm {
    u16	samplerate; /*!< 采样率 */
    u8	onoff; 		 /*!< 初始化状态设置， 开/关 */
    struct __stream_llns_parm llns_parm;
    int (*data_handle)(struct audio_stream_entry *entry,  struct audio_data_frame *in);/*!< 该数据流数据处理前数据回调,可以实现数据检测及做特殊数字处理 */
};

struct adapter_ch_sw_parm {
    u16 buf_len;
    u8  out_ch_type;
    int (*data_handle)(struct audio_stream_entry *entry,  struct audio_data_frame *in);/*!< 该数据流数据处理前数据回调,可以实现数据检测及做特殊数字处理 */
};


struct adapter_stream_fmt {
    u8 attr;														/*!< 数据流参数属性类型选择, 请参考enum adapter_stream_attr定义 */
    union {
        struct adapter_src_parm src; 								/*!< 变采样节点参数 */
        struct adapter_sync_parm sync;								/*!< 同步节点参数 */
        struct adapter_digital_vol_parm digital_vol;				/*!< 数字音量节点参数 */
        struct adapter_encode_stream_parm encoder;					/*!< 编码节点参数 */
        struct adapter_debug_entry_parm debug_entry;				/*!< 调试节点参数 */
        struct adapter_dac_parm dac;								/*!< dac节点参数 */
        struct adapter_noisegate_parm noisegate;					/*!< 噪声门限节点参数 */
        struct adapter_HighSampleRateSingleMicSystem_parm denoise;	/*!< 降噪节点参数 */
        struct adapter_eq_parm miceq;								/*!< mic eq节点参数 */
        struct adapter_eq_parm musiceq;								/*!< 音乐 eq节点参数 */
        struct adapter_dccs_parm dccs;								/*!< dccs节点参数 */
        struct adapter_echo_parm echo;								/*!< 混响节点参数 */
        struct adapter_plate_reverb_parm plate_reverb;							/*!< 混响节点参数 */
        struct adapter_auto_mute_parm auto_mute;					/*!< 自动mute节点参数 */
        struct adapter_howling_parm howling;                        /*!< 啸叫抑制节点参数 */
        struct adapter_dac1_parm dac1;								/*!< dac节点参数 */
        struct adapter_stream_demo_parm demo;						/*!< demo节点参数 */
        struct adapter_llns_parm llns;								/*!< 低延时降噪节点参数 */
        struct adapter_ch_sw_parm ch_sw;							/*!< 声道转换节点参数 */
    } value;
};

struct adapter_audio_stream {
    u8	vol_limit;												/*!< 音量范围，如：vol_limit = 100, 音量调节的范围为0~100,设置音量时候会转换成本地音量表的音量范围 */
    struct audio_stream *stream;								/*!< 音频流控制句柄 */
    struct __stream_sync *sync;									/*!< 同步节点控制句柄 */
    struct __stream_src *src;									/*!< 变采样节点控制句柄 */
    void *dvol;													/*!< 数字音量节点控制句柄 */
    struct adapter_encoder *encoder;							/*!< 编码节点控制句柄 */
    struct __stream_entry *debug_entry;							/*!< 调试节点控制句柄 */
    struct audio_dac_channel *dac;								/*!< dac节点控制句柄 */
    NOISEGATE_API_STRUCT *noisegate;							/*!< 噪声门限节点控制句柄 */
    struct __stream_HighSampleRateSingleMicSystem *denoise;		/*!< 降噪节点控制句柄 */
#if defined(CONFIG_EFFECT_CORE_V2_ENABLE)

    struct audio_eq *miceq;									/*!< mic eq节点控制句柄 */
    struct audio_eq *musiceq;
#else
    struct audio_eq_drc *miceq;									/*!< mic eq节点控制句柄 */
    struct audio_eq_drc *musiceq;								/*!< music eq节点控制句柄 */
    struct audio_eq_drc *mic_drc;
#endif
    struct stream_eq_dccs *dccs;								/*!< dccs节点控制句柄 */
    ECHO_API_STRUCT *echo;										/*!< 混响节点控制句柄 */
#if defined(CONFIG_EFFECT_CORE_V2_ENABLE)
    PLATE_REVERB_API_STRUCT *plate_reverb;										/*!< 混响节点控制句柄 */
#endif
    void *auto_mute;											/*!< 自动mute节点控制句柄 */
    struct channel_switch *ch_sw;								/*!< 声道转换节点控制句柄 */
    // HOWLING_API_STRUCT *howling;								[>!< 啸叫抑制节点控制句柄 <]
    HOWLING_API_STRUCT 		*howling_ps;
    HOWLING_API_STRUCT 		*notch_howling;
    voice_changer_hdl       *voice_changer;


    struct audio_sound_dac *dac1;
    struct __stream_demo_hdl *demo;
    struct __stream_llns *llns;								/*!< 低延时降噪控制句柄 */

    //ex
    struct audio_sound_dac *ex_dac1;
    struct audio_dac_channel *ex_dac;							/*!< dac节点控制句柄 */
    struct channel_switch *ex_channel_zoom;
    struct __stream_src *ex_src;									/*!< 变采样节点控制句柄 */
};

/**
 * @brief 适配器音频流关闭
 *
 * @param  	audio 音频流控制句柄二重指针
 */
void adapter_audio_stream_close(struct adapter_audio_stream **audio);
/**
 * @brief 适配器音频流关闭
 *
 * @param  	list 适配器节点参数配置表
 * @param  	list_num 适配器节点参数配置表大小， 即节点个数
 * @param  	decoder 解码器控制句柄
 * @param  	media_parm 音频媒体参数
 * @return 	音频流控制句柄
 */
struct adapter_audio_stream *adapter_audio_stream_open(
    struct adapter_stream_fmt *list,
    u8 list_num,
    struct audio_decoder *decoder,
    void (*resume)(void *priv),
    struct adapter_media_parm *media_parm);

/**
 * @brief 适配器音频流关闭
 *
 * @param  	list 适配器节点参数配置表
 * @param  	list_num 适配器节点参数配置表大小， 即节点个数
 * @param  	decoder 解码器控制句柄
 * @param  	media_parm 音频媒体参数
 * @return 	音频流控制句柄
 */
struct adapter_audio_stream *adapter_audio_mixer_stream_open(
    struct adapter_stream_fmt *list,
    u8 list_num,
    struct audio_mixer *mix,
    struct adapter_media_parm *media_parm);
/**
 * @brief 适配器音频流数字音量设置
 *
 * @param  	audio 音频流控制句柄
 * @param  	channel 设置的通道， 如:BIT(0)为设置左声道， BIT(1)为右声道， BIT(0)|BIT(1)为同时设置左右声道
 * @param  	vol 音量值
 */
void adapter_audio_stream_set_vol(struct adapter_audio_stream *audio, u32 channel, u8 vol);
/**
 * @brief 适配器音频流单麦降噪参数设置
 *
 * @param  	audio 音频流控制句柄
 * @param  	parm 降噪参数
 */
void adapter_audio_stream_denoise_set_parm(struct adapter_audio_stream *audio, struct __stream_HighSampleRateSingleMicSystem_parm *parm);
/**
 * @brief 适配器音频流单麦降噪开关
 *
 * @param  	audio 音频流控制句柄
 * @param  	onoff 降噪开关
 */
void adapter_audio_stream_denoise_switch(struct adapter_audio_stream *audio, u8 onoff);
/**
 * @brief 适配器音频流DAC模拟音量设置
 *
 * @param  	audio 音频流控制句柄
 * @param  	dac_gain DAC增益
 */
void adapter_audio_stream_dac_gain_set(struct adapter_audio_stream *audio, u8 dac_gain);
#endif//__ADAPTER_AUDIO_STREAM_H___

