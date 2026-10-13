#ifndef __RECORDER_H__
#define __RECORDER_H__

#include "generic/typedef.h"
#include "recorder_coder.h"

enum {
    RECORDER_CALLBACK_ACTION_START_OK = 0x0,
    RECORDER_CALLBACK_ACTION_ERR,
};

//参数
struct __recorder_parm {
    struct audio_fmt   				*coder;		//编码相关参数
    char 							*dev_logo;	//录音设备盘符
    char 							*folder;	//录音文件夹
    char 							*filename;	//录音文件名
    u16								filename_len;
    struct audio_encoder_task 		*encode_task;
};

//录音数据源写接口
int recorder_pcm_data_write(void *data, u16 len);
//录音启动
int recorder_start(struct __recorder_parm *parm);
//录音停止
void recorder_stop(void);
//录音/暂停
int recorder_pp(void);
//录音获取状态，（状态由recorder_encode提供）
int recorder_get_status(void);
//获取录音标签
int record_get_mark(u8 *buf, u16 len);
//获取最新录音文件簇号
int record_get_newest_file_sclust(void);
//获取最新录音文件大小
int record_get_newest_file_size(void);
//获取录音文件句柄
FILE *recorder_get_cur_file_hdl();
//获取录音时间
u32 recorder_get_time(void);

#endif//__RECORDER_H__

