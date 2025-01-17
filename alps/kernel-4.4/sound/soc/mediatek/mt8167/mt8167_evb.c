/*
 * mt8167_evb.c  --  MT8167 EVB ALSA SoC machine driver
 *
 * Copyright (c) 2016 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/module.h>
#include <linux/delay.h>
#include <sound/soc.h>

enum PINCTRL_PIN_STATE {
	PIN_STATE_DEFAULT = 0,
	PIN_STATE_EXTAMP_ON,
	PIN_STATE_EXTAMP_OFF,
	PIN_STATE_AUDSWH_EN_ON,
	PIN_STATE_AUDSWH_EN_OFF,
	PIN_STATE_AUDSWH_IN_ON,
	PIN_STATE_AUDSWH_IN_OFF,
	PIN_STATE_MAX
};

struct mt8167_evb_priv {
	struct pinctrl *pinctrl;
	struct pinctrl_state *pin_states[PIN_STATE_MAX];
	uint32_t ext_spk_amp_warmup_time_us;
	uint32_t ext_spk_amp_shutdown_time_us;
	uint32_t hp_spk_amp_warmup_time_us;
	uint32_t hp_spk_amp_shutdown_time_us;
	long ext_spk_amp_en;
	long ext_hp_amp_en;
	long aud_swh_in;
};

static const char * const mt8167_evb_pinctrl_pin_str[PIN_STATE_MAX] = {
	"default",
	"extamp_on",
	"extamp_off",
	"audswh_en_on",
	"audswh_en_off",
	"audswh_in_on",
	"audswh_in_off"
};

static char const *aud_swh_text[] = {"Off",
					"Mode_1", "Mode_2", "Mode_3", "Mode_4", "Mode_5",
					"Mode_6", "Mode_7", "Mode_8", "Mode_9", "Mode_10"};

static void mt8167_evb_ext_spk_amp_turn_on(struct snd_soc_card *card)
{
	struct mt8167_evb_priv *card_data = snd_soc_card_get_drvdata(card);
	int ret = 0;

	if (IS_ERR(card_data->pin_states[PIN_STATE_EXTAMP_ON]))
		return;

	ret = pinctrl_select_state(card_data->pinctrl,
		card_data->pin_states[PIN_STATE_EXTAMP_ON]);
	if (ret)
		dev_err(card->dev, "%s failed to select state %d\n",
			__func__, ret);

	if (card_data->ext_spk_amp_warmup_time_us > 0)
		usleep_range(card_data->ext_spk_amp_warmup_time_us,
			card_data->ext_spk_amp_warmup_time_us + 1);
}

static void mt8167_evb_ext_spk_amp_turn_off(struct snd_soc_card *card)
{
	struct mt8167_evb_priv *card_data = snd_soc_card_get_drvdata(card);
	int ret = 0;

	if (IS_ERR(card_data->pin_states[PIN_STATE_EXTAMP_OFF]))
		return;

	ret = pinctrl_select_state(card_data->pinctrl,
		card_data->pin_states[PIN_STATE_EXTAMP_OFF]);
	if (ret)
		dev_err(card->dev, "%s failed to select state %d\n",
			__func__, ret);

	if (card_data->ext_spk_amp_shutdown_time_us > 0)
		usleep_range(card_data->ext_spk_amp_shutdown_time_us,
			card_data->ext_spk_amp_shutdown_time_us + 1);
}

/* Ext Spk Amp Switch */
static int mt8167_evb_ext_spk_amp_get(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_card *card = snd_kcontrol_chip(kcontrol);
	struct mt8167_evb_priv *card_data = snd_soc_card_get_drvdata(card);

	ucontrol->value.integer.value[0] = card_data->ext_spk_amp_en;

	return 0;
}

static int mt8167_evb_ext_spk_amp_put(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_card *card = snd_kcontrol_chip(kcontrol);
	struct mt8167_evb_priv *card_data = snd_soc_card_get_drvdata(card);

	card_data->ext_spk_amp_en = ucontrol->value.integer.value[0];

	if (card_data->ext_spk_amp_en)
		mt8167_evb_ext_spk_amp_turn_on(card);
	else
		mt8167_evb_ext_spk_amp_turn_off(card);

	return 0;
}

static void mt8167_evb_ext_hp_amp_turn_on(struct snd_soc_card *card)
{
	struct mt8167_evb_priv *card_data = snd_soc_card_get_drvdata(card);
	int ret = 0;

	if (IS_ERR(card_data->pin_states[PIN_STATE_EXTAMP_ON]))
		return;

	ret = pinctrl_select_state(card_data->pinctrl,
		card_data->pin_states[PIN_STATE_EXTAMP_ON]);
	if (ret)
		dev_err(card->dev, "%s failed to select state %d\n",
			__func__, ret);

	if (card_data->hp_spk_amp_warmup_time_us > 0)
		usleep_range(card_data->hp_spk_amp_warmup_time_us,
			card_data->hp_spk_amp_warmup_time_us + 1);
}

static void mt8167_evb_ext_hp_amp_turn_off(struct snd_soc_card *card)
{
	struct mt8167_evb_priv *card_data = snd_soc_card_get_drvdata(card);
	int ret = 0;

	if (IS_ERR(card_data->pin_states[PIN_STATE_EXTAMP_OFF]))
		return;

	ret = pinctrl_select_state(card_data->pinctrl,
		card_data->pin_states[PIN_STATE_EXTAMP_OFF]);
	if (ret)
		dev_err(card->dev, "%s failed to select state %d\n",
			__func__, ret);

	if (card_data->hp_spk_amp_shutdown_time_us > 0)
		usleep_range(card_data->hp_spk_amp_shutdown_time_us,
			card_data->hp_spk_amp_shutdown_time_us + 1);
}

/* Ext HP Amp Switch */
static int mt8167_evb_ext_hp_amp_get(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_card *card = snd_kcontrol_chip(kcontrol);
	struct mt8167_evb_priv *card_data = snd_soc_card_get_drvdata(card);

	ucontrol->value.integer.value[0] = card_data->ext_hp_amp_en;

	return 0;
}

static int mt8167_evb_ext_hp_amp_put(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_card *card = snd_kcontrol_chip(kcontrol);
	struct mt8167_evb_priv *card_data = snd_soc_card_get_drvdata(card);
	int i = 0;

	card_data->ext_hp_amp_en = ucontrol->value.integer.value[0];

	if (card_data->ext_hp_amp_en == 0) {
		mt8167_evb_ext_hp_amp_turn_off(card);
	} else {
		i = ucontrol->value.integer.value[0];
		for (; i > 0; i--) {
			mt8167_evb_ext_hp_amp_turn_off(card);
			udelay(2);
			mt8167_evb_ext_hp_amp_turn_on(card);
			udelay(2);
		}
	}

	return 0;
}

static void mt8167_evb_aud_swh_in_turn_on(struct snd_soc_card *card)
{
	struct mt8167_evb_priv *card_data = snd_soc_card_get_drvdata(card);
	int ret = 0;

	if (IS_ERR(card_data->pin_states[PIN_STATE_AUDSWH_IN_ON]))
		return;

	ret = pinctrl_select_state(card_data->pinctrl,
		card_data->pin_states[PIN_STATE_AUDSWH_IN_ON]);
	if (ret)
		dev_err(card->dev, "%s failed to select state %d\n",
			__func__, ret);

	if (card_data->hp_spk_amp_warmup_time_us > 0)
		usleep_range(card_data->hp_spk_amp_warmup_time_us,
			card_data->hp_spk_amp_warmup_time_us + 1);
}

static void mt8167_evb_aud_swh_in_turn_off(struct snd_soc_card *card)
{
	struct mt8167_evb_priv *card_data = snd_soc_card_get_drvdata(card);
	int ret = 0;

	if (IS_ERR(card_data->pin_states[PIN_STATE_AUDSWH_IN_OFF]))
		return;

	ret = pinctrl_select_state(card_data->pinctrl,
		card_data->pin_states[PIN_STATE_AUDSWH_IN_OFF]);
	if (ret)
		dev_err(card->dev, "%s failed to select state %d\n",
			__func__, ret);

	if (card_data->hp_spk_amp_shutdown_time_us > 0)
		usleep_range(card_data->hp_spk_amp_shutdown_time_us,
			card_data->hp_spk_amp_shutdown_time_us + 1);
}

/* AUDIO Switch IN*/
static int mt8167_evb_aud_swh_get(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_card *card = snd_kcontrol_chip(kcontrol);
	struct mt8167_evb_priv *card_data = snd_soc_card_get_drvdata(card);

	ucontrol->value.integer.value[0] = card_data->aud_swh_in;

	return 0;
}

static int mt8167_evb_aud_swh_put(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_card *card = snd_kcontrol_chip(kcontrol);
	struct mt8167_evb_priv *card_data = snd_soc_card_get_drvdata(card);

	card_data->aud_swh_in = ucontrol->value.integer.value[0];

	if (card_data->aud_swh_in)
		mt8167_evb_aud_swh_in_turn_on(card);
	else
		mt8167_evb_aud_swh_in_turn_off(card);

	return 0;
}

static void mt8167_evb_audio_switch_enable(struct snd_soc_card *card, bool enable)
{
	struct mt8167_evb_priv *card_data = snd_soc_card_get_drvdata(card);
	int ret = 0;

	if (IS_ERR(card_data->pin_states[PIN_STATE_AUDSWH_EN_ON]))
		return;
	if (enable) {
		ret = pinctrl_select_state(card_data->pinctrl,
			card_data->pin_states[PIN_STATE_AUDSWH_EN_ON]);
		if (ret)
			dev_err(card->dev, "%s failed to select state %d\n",
				__func__, ret);
	} else {
		ret = pinctrl_select_state(card_data->pinctrl,
			card_data->pin_states[PIN_STATE_AUDSWH_EN_OFF]);
		if (ret)
			dev_err(card->dev, "%s failed to select state %d\n",
				__func__, ret);
	}

	if (card_data->hp_spk_amp_warmup_time_us > 0)
		usleep_range(card_data->hp_spk_amp_warmup_time_us,
			card_data->hp_spk_amp_warmup_time_us + 1);
}

static const struct soc_enum mt8167_evb_enum[] = {
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(aud_swh_text),
				aud_swh_text),
};

static const struct snd_kcontrol_new mt8167_evb_controls[] = {
	/* Ext Spk Amp Switch */
	SOC_SINGLE_BOOL_EXT("Ext Spk Amp Switch",
		0,
		mt8167_evb_ext_spk_amp_get,
		mt8167_evb_ext_spk_amp_put),
	/* Ext HP Amp Switch */
	SOC_ENUM_EXT("Ext HP Amp Switch",
		mt8167_evb_enum[0],
		mt8167_evb_ext_hp_amp_get,
		mt8167_evb_ext_hp_amp_put),
	SOC_SINGLE_BOOL_EXT("Ext Aud Swh Switch",
		0,
		mt8167_evb_aud_swh_get,
		mt8167_evb_aud_swh_put),
};

static int mt8167_evb_mic1_event(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_dapm_context *dapm = w->dapm;
	struct snd_soc_card *card = dapm->card;

	dev_dbg(card->dev, "%s, event %d\n", __func__, event);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		break;
	case SND_SOC_DAPM_POST_PMD:
		break;
	default:
		break;
	}

	return 0;
}

static int mt8167_evb_mic2_event(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_dapm_context *dapm = w->dapm;
	struct snd_soc_card *card = dapm->card;

	dev_dbg(card->dev, "%s, event %d\n", __func__, event);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		break;
	case SND_SOC_DAPM_POST_PMD:
		break;
	default:
		break;
	}

	return 0;
}

static int mt8167_evb_headset_mic_event(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_dapm_context *dapm = w->dapm;
	struct snd_soc_card *card = dapm->card;

	dev_dbg(card->dev, "%s, event %d\n", __func__, event);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		break;
	case SND_SOC_DAPM_POST_PMD:
		break;
	default:
		break;
	}

	return 0;
}

/* HP Ext Amp Switch */
static const struct snd_kcontrol_new mt8167_evb_hp_ext_amp_switch_ctrl =
	SOC_DAPM_SINGLE_VIRT("Switch", 1);

/* HP Spk Amp */
static int mt8167_evb_hp_spk_amp_event(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_dapm_context *dapm = w->dapm;
	struct snd_soc_card *card = dapm->card;

	dev_dbg(card->dev, "%s, event %d\n", __func__, event);

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		mt8167_evb_ext_hp_amp_turn_on(card);
		break;
	case SND_SOC_DAPM_PRE_PMD:
		mt8167_evb_ext_hp_amp_turn_off(card);
		break;
	default:
		break;
	}

	return 0;
}

/* LINEOUT Ext Amp Switch */
static const struct snd_kcontrol_new mt8167_evb_lineout_ext_amp_switch_ctrl =
	SOC_DAPM_SINGLE_VIRT("Switch", 1);

/* Ext Spk Amp */
static int mt8167_evb_ext_spk_amp_wevent(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_dapm_context *dapm = w->dapm;
	struct snd_soc_card *card = dapm->card;

	dev_dbg(card->dev, "%s, event %d\n", __func__, event);

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		mt8167_evb_ext_spk_amp_turn_on(card);
		break;
	case SND_SOC_DAPM_PRE_PMD:
		mt8167_evb_ext_spk_amp_turn_off(card);
		break;
	default:
		break;
	}

	return 0;
}

static const struct snd_soc_dapm_widget mt8167_evb_dapm_widgets[] = {
	SND_SOC_DAPM_MIC("Mic 1", mt8167_evb_mic1_event),
	SND_SOC_DAPM_MIC("Mic 2", mt8167_evb_mic2_event),
	SND_SOC_DAPM_MIC("Headset Mic", mt8167_evb_headset_mic_event),
	SND_SOC_DAPM_SPK("HP Spk Amp", mt8167_evb_hp_spk_amp_event),
	SND_SOC_DAPM_SWITCH("HP Ext Amp",
		SND_SOC_NOPM, 0, 0, &mt8167_evb_hp_ext_amp_switch_ctrl),
	SND_SOC_DAPM_SPK("Ext Spk Amp", mt8167_evb_ext_spk_amp_wevent),
	SND_SOC_DAPM_SWITCH("LINEOUT Ext Amp",
		SND_SOC_NOPM, 0, 0, &mt8167_evb_lineout_ext_amp_switch_ctrl),
};

static const struct snd_soc_dapm_route mt8167_evb_audio_map[] = {
	/* Uplink */

	{"AU_VIN0", NULL, "Mic 1"},
	{"AU_VIN2", NULL, "Mic 2"},

	{"AU_VIN1", NULL, "Headset Mic"},

	/* Downlink */

	/* use external spk amp via AU_HPL/AU_HPR */
	{"HP Ext Amp", "Switch", "AU_HPL"},
	{"HP Ext Amp", "Switch", "AU_HPR"},

	{"HP Spk Amp", NULL, "HP Ext Amp"},
	{"HP Spk Amp", NULL, "HP Ext Amp"},

#ifdef CONFIG_MTK_SPEAKER
	/* use internal spk amp of MT6392 */
	{"MT6392 AIF RX", NULL, "AU_LOL"},
#endif

	/* use external spk amp via AU_LOL */
	{"LINEOUT Ext Amp", "Switch", "AU_LOL"},
	{"Ext Spk Amp", NULL, "LINEOUT Ext Amp"},

	/* ADDA clock - Uplink */
	{"AIF TX", NULL, "AFE_CLK"},
	{"AIF TX", NULL, "AD_CLK"},

	/* ADDA clock - Downlink */
	{"AIF RX", NULL, "AFE_CLK"},
	{"AIF RX", NULL, "DA_CLK"},
};

/* Digital audio interface glue - connects codec <---> CPU */
static struct snd_soc_dai_link mt8167_evb_dais[] = {
	/* Front End DAI links */
	{
		.name = "DL1 Playback",
		.stream_name = "MultiMedia1_PLayback",
		.cpu_dai_name = "DL1",
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.trigger = {
			SND_SOC_DPCM_TRIGGER_POST,
			SND_SOC_DPCM_TRIGGER_POST
		},
		.dynamic = 1,
		.dpcm_playback = 1,
	},
	{
		.name = "VUL Capture",
		.stream_name = "MultiMedia1_Capture",
		.cpu_dai_name = "VUL",
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.trigger = {
			SND_SOC_DPCM_TRIGGER_POST,
			SND_SOC_DPCM_TRIGGER_POST
		},
		.dynamic = 1,
		.dpcm_capture = 1,
	},
	{
		.name = "HDMI",
		.stream_name = "HMDI_PLayback",
		.cpu_dai_name = "HDMI",
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.trigger = {
			SND_SOC_DPCM_TRIGGER_POST,
			SND_SOC_DPCM_TRIGGER_POST
		},
		.dynamic = 1,
		.dpcm_playback = 1,
	},
	{
		.name = "AWB Capture",
		.stream_name = "DL1_AWB_Record",
		.cpu_dai_name = "AWB",
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.trigger = {
			SND_SOC_DPCM_TRIGGER_POST,
			SND_SOC_DPCM_TRIGGER_POST
		},
		.dynamic = 1,
		.dpcm_capture = 1,
	},
	{
		.name = "DL2 Playback",
		.stream_name = "MultiMedia2_PLayback",
		.cpu_dai_name = "DL2",
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.trigger = {
			SND_SOC_DPCM_TRIGGER_POST,
			SND_SOC_DPCM_TRIGGER_POST
		},
		.dynamic = 1,
		.dpcm_playback = 1,
	},
	{
		.name = "DAI Capture",
		.stream_name = "VOIP_Call_BT_Capture",
		.cpu_dai_name = "DAI",
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.trigger = {
			SND_SOC_DPCM_TRIGGER_POST,
			SND_SOC_DPCM_TRIGGER_POST
		},
		.dynamic = 1,
		.dpcm_capture = 1,
	},
	{
		.name = "TDM Capture",
		.stream_name = "TDM_Capture",
		.cpu_dai_name = "TDM_IN",
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.trigger = {
			SND_SOC_DPCM_TRIGGER_POST,
			SND_SOC_DPCM_TRIGGER_POST
		},
		.dynamic = 1,
		.dpcm_capture = 1,
	},
	{
		.name = "VIRTUAL_MRG",
		.stream_name = "MRGRX_PLayback",
		.cpu_dai_name = "VIRTURL_MRG",
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.trigger = {
			SND_SOC_DPCM_TRIGGER_POST,
			SND_SOC_DPCM_TRIGGER_POST
		},
		.dynamic = 1,
		.dpcm_playback = 1,
	},
	{
		.name = "MRGRX_CAPTURE",
		.cpu_dai_name = "AWB",
		.stream_name = "MRGRX_CAPTURE",
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.trigger = {
			SND_SOC_DPCM_TRIGGER_POST,
			SND_SOC_DPCM_TRIGGER_POST
		},
		.dynamic = 1,
		.dpcm_capture = 1,
	},
#ifdef CONFIG_SND_SOC_MTK_BTCVSD
	{
		.name = "BTCVSD_RX",
		.stream_name = "BTCVSD_Capture",
		.cpu_dai_name = "snd-soc-dummy-dai",
		.platform_name = "mt-soc-btcvsd-rx-pcm",
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
	},
	{
		.name = "BTCVSD_TX",
		.stream_name = "BTCVSD_Playback",
		.cpu_dai_name = "snd-soc-dummy-dai",
		.platform_name = "mt-soc-btcvsd-tx-pcm",
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
	},
#endif
	/* Back End DAI links */
	{
		.name = "EXT Codec",
		.cpu_dai_name = "I2S",
		.no_pcm = 1,
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.dai_fmt = SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF |
			   SND_SOC_DAIFMT_CBS_CFS,
		.dpcm_playback = 1,
		.dpcm_capture = 1,
	},
	{
		.name = "2ND EXT Codec",
		.cpu_dai_name = "2ND I2S",
		.no_pcm = 1,
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.dai_fmt = SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF |
			   SND_SOC_DAIFMT_CBS_CFS,
		.dpcm_playback = 1,
		.dpcm_capture = 1,
	},
	{
		.name = "MTK Codec",
		.cpu_dai_name = "INT ADDA",
		.no_pcm = 1,
		.codec_name = "mt8167-codec",
		.codec_dai_name = "mt8167-codec-dai",
		.dpcm_playback = 1,
		.dpcm_capture = 1,
	},
	{
		.name = "HDMI BE",
		.cpu_dai_name = "HDMIO",
		.no_pcm = 1,
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.dai_fmt = SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF |
			   SND_SOC_DAIFMT_CBS_CFS,
		.dpcm_playback = 1,
	},
	{
		.name = "DL BE",
		.cpu_dai_name = "DL Input",
		.no_pcm = 1,
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.dpcm_capture = 1,
	},
	{
		.name = "MRG BT BE",
		.cpu_dai_name = "MRG BT",
		.no_pcm = 1,
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.dpcm_playback = 1,
		.dpcm_capture = 1,
	},
	{
		.name = "PCM0 BE",
		.cpu_dai_name = "PCM0",
		.no_pcm = 1,
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.dpcm_playback = 1,
		.dpcm_capture = 1,
	},
	{
		.name = "TDM IN BE",
		.cpu_dai_name = "TDM_IN_IO",
		.no_pcm = 1,
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.dai_fmt = SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_IB_IF |
			   SND_SOC_DAIFMT_CBS_CFS,
		.dpcm_capture = 1,
	},
};

#ifdef CONFIG_MTK_SPEAKER
static struct snd_soc_aux_dev mt8167_evb_aux_dev[] = {
	{
		.name = "MT6392",
		.codec_name = "mt6392-codec",
	},
};
#endif

static struct snd_soc_card mt8167_evb_card = {
	.name = "mt-snd-card",
	.owner = THIS_MODULE,
	.dai_link = mt8167_evb_dais,
	.num_links = ARRAY_SIZE(mt8167_evb_dais),
	.controls = mt8167_evb_controls,
	.num_controls = ARRAY_SIZE(mt8167_evb_controls),
	.dapm_widgets = mt8167_evb_dapm_widgets,
	.num_dapm_widgets = ARRAY_SIZE(mt8167_evb_dapm_widgets),
	.dapm_routes = mt8167_evb_audio_map,
	.num_dapm_routes = ARRAY_SIZE(mt8167_evb_audio_map),
#ifdef CONFIG_MTK_SPEAKER
	.aux_dev = mt8167_evb_aux_dev,
	.num_aux_devs = ARRAY_SIZE(mt8167_evb_aux_dev),
#endif
};

static int mt8167_evb_gpio_probe(struct snd_soc_card *card)
{
	struct mt8167_evb_priv *card_data;
	int ret = 0;
	int i;

	card_data = snd_soc_card_get_drvdata(card);

	card_data->pinctrl = devm_pinctrl_get(card->dev);
	if (IS_ERR(card_data->pinctrl)) {
		ret = PTR_ERR(card_data->pinctrl);
		dev_err(card->dev, "%s pinctrl_get failed %d\n",
			__func__, ret);
		goto exit;
	}

	for (i = 0 ; i < PIN_STATE_MAX ; i++) {
		card_data->pin_states[i] =
			pinctrl_lookup_state(card_data->pinctrl,
				mt8167_evb_pinctrl_pin_str[i]);
		if (IS_ERR(card_data->pin_states[i])) {
			ret = PTR_ERR(card_data->pin_states[i]);
			dev_warn(card->dev, "%s Can't find pinctrl state %s %d\n",
				__func__, mt8167_evb_pinctrl_pin_str[i], ret);
		}
	}

	/* default state */
	if (!IS_ERR(card_data->pin_states[PIN_STATE_DEFAULT])) {
		ret = pinctrl_select_state(card_data->pinctrl,
				card_data->pin_states[PIN_STATE_DEFAULT]);
		if (ret) {
			dev_err(card->dev, "%s failed to select state %d\n",
				__func__, ret);
			goto exit;
		}
	}
	mt8167_evb_audio_switch_enable(card, true);

exit:
	return ret;
}

static int mt8167_evb_dev_probe(struct platform_device *pdev)
{
	struct snd_soc_card *card = &mt8167_evb_card;
	struct device *dev = &pdev->dev;
	struct device_node *platform_node;
	int ret, i;
	struct mt8167_evb_priv *card_data;

	platform_node = of_parse_phandle(dev->of_node, "mediatek,platform", 0);
	if (!platform_node) {
		dev_err(dev, "Property 'platform' missing or invalid\n");
		return -EINVAL;
	}

	for (i = 0; i < card->num_links; i++) {
		if (mt8167_evb_dais[i].platform_name)
			continue;
		mt8167_evb_dais[i].platform_of_node = platform_node;
	}

	card->dev = dev;

	card_data = devm_kzalloc(dev, sizeof(struct mt8167_evb_priv),
				GFP_KERNEL);
	if (!card_data) {
		ret = -ENOMEM;
		dev_err(dev, "%s allocate card private data fail %d\n",
			__func__, ret);
		return ret;
	}

	snd_soc_card_set_drvdata(card, card_data);

	mt8167_evb_gpio_probe(card);

	of_property_read_u32(dev->of_node,
			"mediatek,ext-spk-amp-warmup-time-us",
			&card_data->ext_spk_amp_warmup_time_us);

	of_property_read_u32(dev->of_node,
			"mediatek,ext-spk-amp-shutdown-time-us",
			&card_data->ext_spk_amp_shutdown_time_us);

	of_property_read_u32(dev->of_node,
			 "mediatek,hp-spk-amp-warmup-time-us",
			 &card_data->hp_spk_amp_warmup_time_us);

	of_property_read_u32(dev->of_node,
			 "mediatek,hp-spk-amp-shutdown-time-us",
			 &card_data->hp_spk_amp_shutdown_time_us);

	ret = devm_snd_soc_register_card(dev, card);
	if (ret)
		dev_err(dev, "%s snd_soc_register_card fail %d\n",
			__func__, ret);

	return ret;
}

static const struct of_device_id mt8167_evb_dt_match[] = {
	{ .compatible = "mediatek,mt8167-mt6392", },
	{ }
};
MODULE_DEVICE_TABLE(of, mt8167_evb_dt_match);

static struct platform_driver mt8167_evb_mach_driver = {
	.driver = {
		   .name = "mt8167-mt6392",
		   .of_match_table = mt8167_evb_dt_match,
#ifdef CONFIG_PM
		   .pm = &snd_soc_pm_ops,
#endif
	},
	.probe = mt8167_evb_dev_probe,
};

module_platform_driver(mt8167_evb_mach_driver);

/* Module information */
MODULE_DESCRIPTION("MT8167 ALSA SoC machine driver");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:mt8167-mt6392");

