#include "app_config.h"


SECTIONS
{
    .text : ALIGN(4)
    {
        gsensor_dev_begin = .;
        KEEP(*(.gsensor_dev))
        gsensor_dev_end = .;

        fm_dev_begin = .;
        KEEP(*(.fm_dev))
        fm_dev_end = .;

        fm_emitter_dev_begin = .;
        KEEP(*(.fm_emitter_dev))
        fm_emitter_dev_end = .;

        storage_device_begin = .;
        KEEP(*(.storage_device))
        storage_device_end = .;

        p11_event_handler_begin = .;
        KEEP(*(.p11_irq_handler))
        p11_event_handler_end = .;

		. = ALIGN(4);
		adapter_idev_begin = .;
		KEEP(*(.adapter_idev))
		adapter_idev_end = .;

		. = ALIGN(4);
		adapter_odev_begin = .;
		KEEP(*(.adapter_odev))
		adapter_odev_end = .;

		. = ALIGN(4);
		adapter_enc_begin = .;
		KEEP(*(.adapter_enc))
		adapter_enc_end = .;


		. = ALIGN(4);
		adapter_decoder_begin = .;
		KEEP(*(.adapter_decoder))
		adapter_decoder_end = .;

		. = ALIGN(4);
		adapter_encoder_begin = .;
		KEEP(*(.adapter_encoder))
		adapter_encoder_end = .;

		. = ALIGN(32);
    }

	.data ALIGN(32):
	{
	} > ram0

	.bss ALIGN(32):
    {
		*(.usb_ep0)
#if (USB_MEM_NO_USE_OVERLAY_EN)
            *(.usb_audio_play_dma)
            *(.usb_audio_rec_dma)
            *(.uac_rx)
            *(.mass_storage)

            *(.usb_msd_dma)
            *(.usb_hid_dma)
            *(.usb_iso_dma)
            *(.usb_cdc_dma)
            *(.uac_var)
            *(.usb_config_var)
            *(.cdc_var)
#endif
    } > ram0

	.data_code ALIGN(32):
	{

#if (TCFG_ENC_JLA_ENABLE || TCFG_DEC_JLA_ENABLE)
		*(.jla_codec_ari_c_code)
		*(.jla_codec_e_code)
		*(.jla_codec_d_code)
		*(.jla_codec_c_const)
		*(.jla_codec_c_code)
#endif

	} > ram0

}
