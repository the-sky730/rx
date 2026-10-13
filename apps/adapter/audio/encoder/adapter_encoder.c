#include "adapter_encoder.h"

static struct audio_encoder_task adapter_encoder_task = {0};

void adapter_encoder_init(void)
{
    int err = audio_encoder_task_create(&adapter_encoder_task, "audio_enc");
}

struct audio_encoder_task *adapter_encoder_get_task_handle(void)
{
    return 	&adapter_encoder_task;
}

struct adapter_encoder *adapter_encoder_open(struct adapter_encoder_fmt *fmt, struct adapter_media_parm *media_parm)
{
    if (fmt == NULL) {
        return NULL;
    }
    struct adapter_encoder *enc = NULL;
    list_for_each_adapter_encoder(enc) {
        if (enc->enc_type == fmt->enc_type) {
            if (enc->open) {
                enc->open(fmt, media_parm);
                return enc;
            }
        }
    }
    return NULL;
}

void adapter_encoder_close(struct adapter_encoder *enc)
{
    if (enc && enc->close) {
        enc->close();
    }
}


struct audio_stream_entry *adapter_encoder_get_stream_entry(struct adapter_encoder *enc)
{
    if (enc && enc->get_stream_entry) {
        return enc->get_stream_entry();
    }
    return NULL;
}



