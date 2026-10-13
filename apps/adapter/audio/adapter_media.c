#include "adapter_media.h"
#include "adapter_process.h"

void adapter_media_init(void)
{
    adapter_encoder_init();
    adapter_decoder_init();
}


struct adapter_media *adapter_media_open(struct adapter_media_config *config)
{
    struct adapter_media *media = zalloc(sizeof(struct adapter_media));
    if (media) {
        ASSERT(config, "%s, config err!!\n", __FUNCTION__);
        media->config = config;
    }

    return media;
}

void adapter_media_close(struct adapter_media **hdl)
{
    if (hdl && (*hdl)) {
        free(*hdl);
        *hdl = NULL;
    }
}


void adapter_media_stop(struct adapter_media *media)
{
    if (media) {
        //media->status = 0;
        adapter_decoder_close(media->updecode);
        adapter_decoder_close(media->downdecode);
        media->updecode = NULL;
        media->downdecode = NULL;
        g_printf("%s, %d\n", __FUNCTION__, __LINE__);
        clk_set("sys", 48000000);
    }
}


int adapter_media_start(struct adapter_media *media)
{
    if (media == NULL) {
        return -1;
    }
    if (media->media_sel >= media->config->list_num) {
        //printf("%s, %d\n", __FUNCTION__, __LINE__);
        return -1;
    }
#if (WIRELESS_CLK == 144)
    clk_set("sys", 144000000);
#elif (WIRELESS_CLK == 160)
    clk_set("sys", 160000000);
#else
    clk_set("sys", 120000000);
#endif
    struct adapter_media_fmt *fmt = media->config->list + media->media_sel;
    g_f_printf("media->media_sel = %d\n", media->media_sel);
    //upstream
    if (fmt->upstream) {
        g_printf("upstream decode open\n");
        media->updecode = adapter_decoder_open(fmt->upstream, &media->upstream_parm);
    }
    //downstream
    if (fmt->downstream) {
        g_printf("downstream decode open\n");
        media->downdecode = adapter_decoder_open(fmt->downstream, &media->downstream_parm);
    }
    return 0;
}

struct adapter_media_parm *adapter_media_get_upstream_parm_handle(struct adapter_media *media)
{
    if (media) {
        return &media->upstream_parm;
    }
    return NULL;
}

struct adapter_media_parm *adapter_media_get_downstream_parm_handle(struct adapter_media *media)
{
    if (media) {
        return &media->downstream_parm;
    }
    return NULL;
}


