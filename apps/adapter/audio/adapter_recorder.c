#include "adapter_recorder.h"
#include "app_config.h"
#include "recorder_stream.h"
#include "recorder.h"
#include "recorder_file.h"

#if WIRELESS_MIC_RECORDER_ENABLE

#if TCFG_ENC_MP3_ENABLE
#define WIRELESS_MIC_CONFIG_CODING_TYPE 	AUDIO_CODING_MP3//AUDIO_CODING_MP3//AUDIO_CODING_PCM//AUDIO_CODING_WAV //
#else
#define WIRELESS_MIC_CONFIG_CODING_TYPE 	AUDIO_CODING_WAV//AUDIO_CODING_MP3//AUDIO_CODING_PCM//AUDIO_CODING_WAV //
#endif
#define WIRELESS_MIC_CONFIG_BIT_RATE		128
#define WIRELESS_MIC_CONFIG_BLOCK_SIZE		512
#define WIRELESS_MIC_CONFIG_FOLDER_NAME		"JL_REC"
#define WIRELESS_MIC_CONFIG_WAV_FILE_NAME	"JL****.wav"
#define WIRELESS_MIC_CONFIG_MP3_FILE_NAME	"JL****.mp3"

static u8 wl_mic_recorder_status = 0;
u8 wl_mic_get_recorder_status(void)
{
    return wl_mic_recorder_status;
}
static void wl_mic_set_recorder_status(u8 status)
{
    wl_mic_recorder_status = status;
}
void wireless_mic_recorder_start(void)
{
    if (wl_mic_get_recorder_status()) {
        return;
    }
    //recorder参数初始化
    struct __recorder_parm recorder_parm = {0};
    recorder_parm.dev_logo = dev_manager_get_phy_logo(dev_manager_find_active(0));
    printf("recorder_parm.dev_logo = %s", recorder_parm.dev_logo);
    recorder_parm.folder = WIRELESS_MIC_CONFIG_FOLDER_NAME;

#if (WIRELESS_MIC_CONFIG_CODING_TYPE == AUDIO_CODING_MP3)
    //char file_path[] = { '\\', 'U', 0x60, 0x4F, 0x7D, 0x59, 0x40, 0x54, 0x2E, 0, 0x6D, 0, 0x70, 0, 0x33, 0}; //你好呀.mp3;
    //recorder_parm.filename = file_path;
    //recorder_parm.filename_len = sizeof(file_path);//strlen(recorder_parm.filename);
    recorder_parm.filename = WIRELESS_MIC_CONFIG_MP3_FILE_NAME;//后缀跟进实际选择的编码进行设置
    recorder_parm.filename_len = strlen(recorder_parm.filename);
#else
    recorder_parm.filename = WIRELESS_MIC_CONFIG_WAV_FILE_NAME;//后缀跟进实际选择的编码进行设置
    recorder_parm.filename_len = strlen(recorder_parm.filename);
#endif


    struct audio_fmt encode_parm = {0};
    encode_parm.coding_type = WIRELESS_MIC_CONFIG_CODING_TYPE;
    encode_parm.channel = WIRELESS_CODING_CHANNEL_NUM;
    encode_parm.sample_rate = WIRELESS_CODING_SAMPLERATE;

#if (WIRELESS_MIC_CONFIG_CODING_TYPE == AUDIO_CODING_MP3)
    encode_parm.bit_rate = WIRELESS_MIC_CONFIG_BIT_RATE;//非wav格式填写实际码率， wav格式此参数填block_size(256/512/1024/2048)
#else
    encode_parm.bit_rate = WIRELESS_MIC_CONFIG_BLOCK_SIZE;//非wav格式填写实际码率， wav格式此参数填block_size(256/512/1024/2048)
#endif

    recorder_parm.coder = &encode_parm;
    recorder_parm.encode_task = adapter_encoder_get_task_handle();
    //recorder 启动
    int ret = recorder_start(&recorder_parm);
    if (ret) {
        printf("recorder_start fail\n");
        return ;
    }
    wl_mic_set_recorder_status(1);


}
void wireless_mic_recorder_stop(void)
{
    if (!wl_mic_get_recorder_status()) {
        return;
    }
    wl_mic_set_recorder_status(0);
    recorder_stop();
    printf("recorder_close ok!!!!!!!!!!!!!!!!!!!!!!!\n");
}

void wireless_mic_recorder_pcm_data_write(void *data, u16 len)
{
    recorder_pcm_data_write(data, len);
}

#endif
