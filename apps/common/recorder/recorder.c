#include "recorder.h"
#include "recorder_coder.h"
#include "recorder_file.h"
#include "dev_manager.h"
#include "system/includes.h"

//此状态为recorder内部管理
enum {
    RECORDER_STATUS_IDLE = 0x0,
    RECORDER_STATUS_STOP,
    RECORDER_STATUS_PLAY,
};

struct __recorder {
    struct __recorder_coder 	*coder;
    struct __recorder_file		*wfile;
    struct __dev 				*dev;
    volatile u8 				status;
};

struct __recorder *g_recorder = NULL;
#define __this g_recorder
static int recfile_sclust     = 0;
static int recfile_size     = 0;

static OS_MUTEX recorder_mutex;
static void recorder_mutex_init(void)
{
    static u8 init = 0;
    if (init) {
        return ;
    }
    init = 1;
    os_mutex_create(&recorder_mutex);
}
static void recorder_mutex_enter(void)
{
    recorder_mutex_init();
    os_mutex_pend(&recorder_mutex, 0);
}
static void recorder_mutex_exit(void)
{
    os_mutex_post(&recorder_mutex);
}

static void recorder_release(struct __recorder **hdl)
{
    if (hdl == NULL || *hdl == NULL)	{
        return ;
    }

    struct __recorder *recorder = *hdl;
    recorder->status = RECORDER_STATUS_STOP;

    if (recorder->coder) {
        recorder_coder_stop(recorder->coder);
        recorder_coder_close(&recorder->coder);
    }

    if (recorder->wfile) {
        recorder_file_close(&recorder->wfile);
    }

    free(recorder);
    *hdl = NULL;
}

static int recorder_coder_callback(void *priv, int event, int argc, void *argv)
{
    if (__this == NULL)	{
        return 0;
    }
    int ret = 0;
    switch (event) {
    case RECORDER_CODER_CALLBACK_EVENT_OUTPUT:
        //编码完成的数据回调
        ret = recorder_file_data_write(__this->wfile, argv, argc);
        break;
    case RECORDER_CODER_CALLBACK_EVENT_SET_HEAD:
        //设置头部信息回调
        recorder_file_upadate_head(__this->wfile, argv, argc);
        break;
    }
    return ret;
}

static int recorder_file_callback(void *priv, int event, int argc, void *argv)
{
    if (__this == NULL)	{
        return 0;
    }
    int ret = 0;
    switch (event) {
    case RECORDER_FILE_CALLBACK_EVENT_WRITE_EMPTY:
        //写文件， 没有数据可以写了
        if (__this && (__this->status > RECORDER_STATUS_STOP)) {
            recorder_coder_resume(__this->coder);
        }
        break;
    case RECORDER_FILE_CALLBACK_EVENT_WRITE_ERR:
        //写文件错误
        printf("EVENT_WRITE_ERR post msg!!\n");
        break;
    case RECORDER_FILE_CALLBACK_EVENT_FILE_CLOSE:
        //获取录音文件的簇号用于回播
        if (argc == 2) {
            int *msg = (int *)argv;
            recfile_sclust = msg[0];
            recfile_size =   msg[1];
        }
        break;
    }
    return ret;
}

int recorder_pcm_data_write(void *data, u16 len)
{
    int wlen = 0;
    recorder_mutex_enter();
    if (__this && (__this->status > RECORDER_STATUS_STOP)) {
        wlen = recorder_coder_pcm_data_write(__this->coder, data, len);
    }
    recorder_mutex_exit();
    return wlen;
}

int recorder_start(struct __recorder_parm *parm)
{
    int ret = 0;
    //申请资源
    recfile_sclust     = -1;
    recfile_size       =  0;
    struct __recorder *recorder = zalloc(sizeof(struct __recorder));
    if (recorder == NULL) {
        return -1;
    }
    //增加系统时钟

    //设备选择
    recorder->dev = dev_manager_find_spec(parm->dev_logo, 0);
    if (recorder->dev == NULL) {
        goto __err;
    }

    //文件操作初始化
    char *root_path = dev_manager_get_root_path(recorder->dev);
    recorder->wfile = recorder_file_open(root_path, parm->folder, parm->filename, parm->filename_len);
    if (recorder->wfile == NULL) {
        goto __err;
    }

    //编码初始化
    recorder->coder = recorder_coder_open(parm->coder, parm->encode_task);
    if (recorder->coder == NULL) {
        goto __err;
    }

    //设置编码回调
    recorder_coder_set_callback(recorder->coder, recorder_coder_callback, recorder);
    //设置文件操作回调
    recorder_file_set_callback(recorder->wfile, recorder_file_callback, recorder);
    recorder_file_set_limit(recorder->wfile, 0, 0);

    //启动编码
    ret = recorder_coder_start(recorder->coder);
    if (ret) {
        goto __err;
    }
    //启动写文件
    ret = recorder_file_start(recorder->wfile);
    if (ret) {
        goto __err;
    }

    recorder_mutex_enter();
    __this = recorder;
    __this->status = RECORDER_STATUS_PLAY;
    recorder_mutex_exit();

    r_f_printf("%s, succ!!!\n", __func__);
    return 0;

__err:
    r_f_printf("%s, fail\n", __FUNCTION__);
    recorder_release(&recorder);
    return -1;
}

void recorder_stop(void)
{
    recorder_mutex_enter();
    recorder_release(&__this);
    recorder_mutex_exit();
}

int recorder_pp(void)
{
    int status = 0;
    recorder_mutex_enter();
    if (__this && (__this->status > RECORDER_STATUS_STOP)) {
        status = recorder_coder_pp(__this->coder);
    }
    recorder_mutex_exit();
    return status;
}

int recorder_get_status(void)
{
    int status = 0;
    recorder_mutex_enter();
    if (__this && (__this->status > RECORDER_STATUS_STOP)) {
        status = recorder_coder_get_status(__this->coder);
    }
    recorder_mutex_exit();
    return status;
}

int record_get_mark(u8 *buf, u16 len)
{

    int ret = 0;
    recorder_mutex_enter();
    if (__this && (__this->status > RECORDER_STATUS_STOP)) {
        ret  = recorder_coder_get_mark(__this->coder, buf, len);
    }
    recorder_mutex_exit();
    return ret;
}

int record_get_newest_file_sclust(void)
{
    if (recfile_sclust) {
        return recfile_sclust;
    }
    return -1;
}

int record_get_newest_file_size(void)
{
    if (recfile_size) {
        return recfile_size;
    }
    return 0;
}

FILE *recorder_get_cur_file_hdl()
{
    FILE *file;
    if (__this && (__this->status > RECORDER_STATUS_STOP)) {
        file = recorder_file_get_hdl(__this->wfile);
        return file;
    }
    return NULL;
}

u32 recorder_get_time(void)
{
    u32 time = 0;
    recorder_mutex_enter();
    if (__this && (__this->status > RECORDER_STATUS_STOP)) {
        time = recorder_coder_get_time(__this->coder);
    }
    recorder_mutex_exit();
    return time;
}



