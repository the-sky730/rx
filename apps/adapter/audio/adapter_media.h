#ifndef __ADAPTER_MEDIA_H__
#define __ADAPTER_MEDIA_H__

#include "app_config.h"
#include "generic/typedef.h"
#include "media/includes.h"
#include "adapter_decoder.h"
#include "adapter_encoder.h"

/**************************************************************************************************
  Data Types
**************************************************************************************************/

struct adapter_media_fmt {
    /* --- 媒体流配置单元 --- */
    struct adapter_decoder_fmt	*upstream;	/*!< 上行媒体配置 */
    struct adapter_decoder_fmt 	*downstream;/*!< 下行媒体配置 */
};

struct adapter_media_config {
    /* --- 媒体流配置（表）,静态const参数 --- */
    u8 list_num;							/*!< 媒体组合总数 */
    struct adapter_media_fmt *list;			/*!< 媒体组合列表 */
};

struct adapter_media_parm {
    /* --- 媒体流动态参数 --- */
    u8	start_vol_l;		/*!< 音频启动前的L音量 */
    u8	start_vol_r;		/*!< 音频启动前的R音量 */
    u8	vol_limit;			/*!< 音量范围，如：vol_limit = 100, 音量调节的范围为0~100,设置音量时候会转换成本地音量表的音量范围 */
    u8  ch_num;				/*!< 通道数 */
    u8  ex_output;
    u8  ch_index;			/*!< 第几通道 */
    u16 sample_rate;		/*!< 采样率 */
    u16 coding_type;		/*!< 编码格式 */
    u16	frame_len;			/*!< 帧长度 */
    u16	mixer_output_select;/*!< mixer输出方式：真立体/假立体 */
    void *priv_data;		/*!< 未知私有参数 */
};

struct adapter_media {
    /* --- 媒体控制句柄 --- */
    u8								media_sel;			/*!< 媒体配置选择, 对应adapter_media_config中list的下标 */
    struct adapter_media_config     *config;			/*!< 数据流节点配置, 为const参数 */
    struct adapter_media_parm 		upstream_parm;		/*!< 上行动态参数 */
    struct adapter_media_parm 		downstream_parm;	/*!< 下行动态参数 */
    struct adapter_decoder 			*updecode;			/*!< 上行解码控制句柄 */
    struct adapter_decoder 			*downdecode;		/*!< 下行解码控制句柄 */
};

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/


/**
 * @brief 音频媒体初始化， 只会在上电初始化一次， 固定调用
 */
void adapter_media_init(void);

/**
 * @brief 音频媒体创建,app_main主流程中调用
 *
 * @param  	config 参数配置， 详细请参考struct adapter_media_config结构体定义
 * @return	返回音频媒体总控制句柄
 */
struct adapter_media *adapter_media_open(struct adapter_media_config *config);
/**
 * @brief 音频媒体关闭,app_main主流程中调用
 * @param  	hdl 音频媒体总控制句柄二重指针
 */
void adapter_media_close(struct adapter_media **hdl);
/**
 * @brief 音频媒体停止,process模块主流程中调用
 *
 * @param  	media 音频媒体总控制句柄
 */
void adapter_media_stop(struct adapter_media *media);
/**
 * @brief 音频媒体开始,process模块主流程中调用
 *
 * @param  	media 音频媒体总控制句柄
 * @return	返回0成功， 返回非0失败
 */
int adapter_media_start(struct adapter_media *media);
/**
 * @brief 获取音频媒体上行音频媒体动态参数句柄
 *
 * @param  	media 音频媒体总控制句柄
 * @return	返回非空为音频媒体动态参数结构体指针， 可以在媒体启动前进行修改及更新
 */
struct adapter_media_parm *adapter_media_get_upstream_parm_handle(struct adapter_media *media);
/**
 * @brief 获取音频媒体下行音频媒体动态参数句柄
 *
 * @param  	media 音频媒体总控制句柄
 * @return	返回非空为音频媒体动态参数结构体指针， 可以在媒体启动前进行修改及更新
 */
struct adapter_media_parm *adapter_media_get_downstream_parm_handle(struct adapter_media *media);



#endif//__ADAPTER_MEDIA_H__

