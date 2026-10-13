#ifndef __RECORDER_FILE_H__
#define __RECORDER_FILE_H__

#include "generic/typedef.h"
#include "system/fs/fs.h"

struct __recorder_file;

enum {
    RECORDER_FILE_CALLBACK_EVENT_WRITE_EMPTY = 0x0,
    RECORDER_FILE_CALLBACK_EVENT_WRITE_ERR,
    RECORDER_FILE_CALLBACK_EVENT_FILE_CLOSE,
};

//录音文件操作关闭
void recorder_file_close(struct __recorder_file **hdl);
//录音文件操作打开
struct __recorder_file *recorder_file_open(char *root_path, char *folder, char *name, u16 name_len);
//录音文件操作开始
int recorder_file_start(struct __recorder_file *wfile);
//录音文件操作设置限制条件
void recorder_file_set_limit(struct __recorder_file *wfile, u32 cut_size, u32 limit_size);
//录音文件操作更新头部信息
void recorder_file_upadate_head(struct __recorder_file *wfile, void *head, int len);
//录音文件操作数据写入处理
int recorder_file_data_write(struct __recorder_file *wfile, void *data, int len);
//录音文件操作设置回调处理
void recorder_file_set_callback(struct __recorder_file *wfile, int (*callback)(void *priv, int event, int argc, void *argv), void *priv);
//录音文件句柄
FILE *recorder_file_get_hdl(struct __recorder_file *wfile);

#endif//__RECORDER_FILE_H__

