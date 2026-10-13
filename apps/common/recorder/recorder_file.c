#include "recorder_file.h"
#include "system/includes.h"
#include "app_config.h"

#define RECORDER_FILE_TASK_NAME				"enc_write"
#define RECORDER_FILE_INPUT_SIZE 			(16*1024)
#define RECORDER_FILE_WRITE_UNIT			(512)
#define RECORDER_FILE_HEAD_SIZE_MAX			(128)

struct __recorder_file {
    OS_SEM 					sem;
    FILE 					*file;
    u8 						*input_buf;
    cbuffer_t 				input_cbuf;
    u8 						temp_buf[RECORDER_FILE_WRITE_UNIT];
    u8 						head[RECORDER_FILE_HEAD_SIZE_MAX];
    u16						head_len;
    u32 					cut_size;
    u32 					limit_size;
    volatile u8				status;
    volatile u8				busy;
    volatile u8				err;
    //回调
    void *callback_priv;
    int (*callback)(void *callback_priv, int event, int argc, void *argv);//event查看recorder_file.h回调事件枚举
};

//状态枚举
enum {
    RECORDER_FILE_STATUS_IDLE = 0x0,
    RECORDER_FILE_STATUS_STOP,
    RECORDER_FILE_STATUS_START,
};


static FILE *recorder_file_creat_file(char *root_path, char *folder, char *name, u16 name_len)
{
    FILE *file = NULL;
    u32 path_len = (strlen(root_path) + strlen(folder) + strlen("/") + name_len + 2);
    u8 *path = zalloc(path_len);
    if (path == NULL) {
        return NULL;
    }

    strcat(path, root_path);
    strcat(path, folder);
    strcat(path, "/");
    memcpy(path + strlen(root_path) + strlen(folder) + strlen("/"), name, name_len);
    //strcat(path, name);
    //printf("path = %s\n", path);
    printf("path buf:\n");
    put_buf(path, path_len);
    file = fopen(path, "w+");
    free(path);
    return file;
}

static void recorder_file_close_file(struct __recorder_file *wfile)
{
    int f_len = fpos(wfile->file);
    if (f_len <= wfile->cut_size) {
        //文件不够砍
        fdelete(wfile->file);
        printf(" < cut_size !!\n");
        return ;
    }
    f_len -= wfile->cut_size;
    if (f_len <= wfile->limit_size) {
        //文件小于限定大小
        fdelete(wfile->file);
        printf(" < limit_size !!\n");
        return ;
    }

    if (wfile->head_len && (wfile->err == 0)) {
        //写头部信息
        if (f_len <= wfile->head_len) {
            //文件小于头部信息大小
            fdelete(wfile->file);
            printf(" < head_size !!\n");
            return ;
        }
        //重新将文件将偏移到文件尾部
        fseek(wfile->file, 0, SEEK_SET);
        fwrite(wfile->file, wfile->head, wfile->head_len);
        fseek(wfile->file, f_len, SEEK_SET);
    }

    if (wfile->callback) {
        //文件关闭回调， 方便外部获取文件信息, 例如做回放处理
        struct vfs_attr attr = {0};
        fget_attrs(wfile->file, &attr);
        int msg[2];
        msg[0] = attr.sclust;
        msg[1] = f_len;
        wfile->callback(wfile->callback_priv, RECORDER_FILE_CALLBACK_EVENT_FILE_CLOSE, 2, msg);
    }
    fclose(wfile->file);
}

static void recorder_file_resume(struct __recorder_file *wfile)
{
    if (wfile == NULL) {
        return ;
    }
    if (wfile->status != RECORDER_FILE_STATUS_START) {
        return ;
    }

    os_sem_set(&wfile->sem, 0);
    os_sem_post(&wfile->sem);
}

static void recorder_write_file_run(struct __recorder_file *wfile)
{
    u8 *addr = NULL;
    u32 data_len = 0;
    int wlen = 0;
    while (1) {
        //尝试cbuf read alloc, 减少内存拷贝的开销
        addr = cbuf_read_alloc(&wfile->input_cbuf, &data_len);
        if (addr && (data_len >= RECORDER_FILE_WRITE_UNIT)) {
            data_len = RECORDER_FILE_WRITE_UNIT;
            wlen = fwrite(wfile->file, addr, RECORDER_FILE_WRITE_UNIT);
            cbuf_read_updata(&wfile->input_cbuf, data_len);
            if (wlen == data_len) {
                continue;
            } else {
                printf("write err 1\n");
                wfile->err = 1;
                break;
            }
        }
        //尝试cbuf read
        data_len = cbuf_read(&wfile->input_cbuf, wfile->temp_buf, RECORDER_FILE_WRITE_UNIT);
        if (data_len) {
            wlen = fwrite(wfile->file, wfile->temp_buf, data_len);
            if (wlen == data_len) {
                continue;
            } else {
                printf("write err 2\n");
                wfile->err = 1;
                break;
            }
        }
        //检查到解码停止， 把剩余不足一个UNIT的数据输出
        if (wfile->status == RECORDER_FILE_STATUS_STOP) {
            data_len = cbuf_get_data_len(&wfile->input_cbuf);
            data_len = cbuf_read(&wfile->input_cbuf, wfile->temp_buf, data_len);
            wlen = fwrite(wfile->file, wfile->temp_buf, data_len);
            if (wlen == data_len) {
                //continue;
            } else {
                printf("write err 3\n");
                wfile->err = 1;
                break;
            }
            //数据输出完毕,退出write流程
            break;
        }
        //cbuf里面的数据消耗完了, 等待外部写入
        if (wfile->callback) {
            //写空之后， 回调给外面接口可以尝试激活编码
            wfile->callback(wfile->callback_priv, RECORDER_FILE_CALLBACK_EVENT_WRITE_EMPTY, 0, NULL);
        }
        os_sem_pend(&wfile->sem, 0);
        if (wfile->status == RECORDER_FILE_STATUS_STOP) {
            printf("stop write!!!!\n");
            continue;//把剩余的数据输出完成
            //break;
        }
    }

    if (wfile->err) {
        if (wfile->callback) {
            //写错误，告知外部要终止文件写操作了
            wfile->callback(wfile->callback_priv, RECORDER_FILE_CALLBACK_EVENT_WRITE_ERR, 0, NULL);
        }
    }
}

void recorder_file_task(void *p)
{
    int rlen;
    struct __recorder_file *wfile = (struct __recorder_file *)p;

    while (1) {
        os_sem_pend(&wfile->sem, 0);
        wfile->busy = 1;
        //从缓冲读取数据写入文件
        if (wfile->status == RECORDER_FILE_STATUS_START) {
            recorder_write_file_run(wfile);
        }
        wfile->busy = 0;
        if (wfile->status == RECORDER_FILE_STATUS_STOP) {
            break;
        }
    }
    printf("task wait kill!!\n");
    while (1) {
        os_time_dly(10000);
    }
}

void recorder_file_close(struct __recorder_file **hdl)
{
    if (hdl == NULL || (*hdl == NULL)) {
        return ;
    }
    struct __recorder_file *wfile = *hdl;
    if (wfile->status == RECORDER_FILE_STATUS_START) {
        //等待线程处理完成, 并删除线程
        wfile->status = RECORDER_FILE_STATUS_STOP;
        while (wfile->busy) {
            os_sem_set(&wfile->sem, 0);
            os_sem_post(&wfile->sem);
            os_time_dly(2);
        }
        printf("%s, wait busy ok\n", __FUNCTION__);

        task_kill(RECORDER_FILE_TASK_NAME);
    }

    //关闭文件处理
    recorder_file_close_file(wfile);

    //释放资源
    if (wfile->input_buf) {
        free(wfile->input_buf);
    }
    free(wfile);
    *hdl = NULL;
}

struct __recorder_file *recorder_file_open(char *root_path, char *folder, char *name, u16 name_len)
{
    struct __recorder_file *wfile = NULL;
    //创建文件
    FILE *file = recorder_file_creat_file(root_path, folder, name, name_len);
    if (file == NULL) {
        goto __err;
    }

    //申请句柄
    wfile = (struct __recorder_file *)zalloc(sizeof(struct __recorder_file));
    if (wfile == NULL) {
        goto __err;
    }

    wfile->busy = 0;
    wfile->status = RECORDER_FILE_STATUS_IDLE;
    wfile->file = file;

    os_sem_create(&wfile->sem, 0);

    //输出缓存初始化
    wfile->input_buf = zalloc(RECORDER_FILE_INPUT_SIZE);
    if (wfile->input_buf == NULL) {
        goto __err;
    }
    cbuf_init(&wfile->input_cbuf, wfile->input_buf, RECORDER_FILE_INPUT_SIZE);

    //创建文件写线程
    int err = task_create(recorder_file_task, wfile, RECORDER_FILE_TASK_NAME);
    if (err) {
        goto __err;
    }

    return wfile;
__err:
    if (file && (wfile == NULL)) {
        fclose(file);
    }
    recorder_file_close(&wfile);
    return NULL;
}

int recorder_file_start(struct __recorder_file *wfile)
{
    if (wfile == NULL) {
        return -1;
    }
    wfile->status = RECORDER_FILE_STATUS_START;
    recorder_file_resume(wfile);
    return 0;
}

void recorder_file_set_limit(struct __recorder_file *wfile, u32 cut_size, u32 limit_size)
{
    if (wfile == NULL) {
        return ;
    }
    wfile->cut_size = cut_size;
    wfile->limit_size = limit_size;
}

void recorder_file_upadate_head(struct __recorder_file *wfile, void *head, int len)
{
    if (wfile == NULL) {
        return ;
    }
    if (len > RECORDER_FILE_HEAD_SIZE_MAX) {
        return ;
    }
    wfile->head_len = len;
    if (len) {
        memcpy(wfile->head, head, len);
    }
}

int recorder_file_data_write(struct __recorder_file *wfile, void *data, int len)
{
    if (wfile == NULL) {
        return 0;
    }
    if (wfile->status != RECORDER_FILE_STATUS_START) {
        return 0;
    }
    int wlen = cbuf_write(&wfile->input_cbuf, data, len);
    if (wlen != len) {
        putchar('!');
    }
    recorder_file_resume(wfile);
    return wlen;
}

void recorder_file_set_callback(struct __recorder_file *wfile, int (*callback)(void *priv, int event, int argc, void *argv), void *priv)
{
    if (wfile == NULL) {
        return ;
    }
    wfile->callback_priv = priv;
    wfile->callback = callback;
}

FILE *recorder_file_get_hdl(struct __recorder_file *wfile)
{
    if (wfile == NULL) {
        return NULL;
    }
    return wfile->file;
}

