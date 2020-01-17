// Ring-fork: Pop

#include <QString>
#include <QFile>
#include <QByteArray>

#define DR_WAV_IMPLEMENTATION
#define MINIAUDIO_IMPLEMENTATION
#include <qt/pop/soundplayer.h>

ma_decoder decoder;
ma_device_config deviceConfig;
ma_device device;

void miniAudioDataCallback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
    ma_decoder* pDecoder = (ma_decoder*)pDevice->pUserData;
    if (pDecoder == NULL) {
        return;
    }

    ma_decoder_read_pcm_frames(pDecoder, pOutput, frameCount);

    (void)pInput;
}

void JustPlay(QString filename) {
    if (ma_device_is_started(&device)) {
        ma_device_stop(&device);
        ma_device_uninit(&device);
        ma_decoder_uninit(&decoder);
    }

    QFile resFile(filename);
    if (!resFile.open(QFile::ReadOnly)) return;
    QByteArray audioBytes = resFile.readAll();
    resFile.close();

    if (ma_decoder_init_memory_wav(audioBytes.constData(), audioBytes.size(), NULL, &decoder) != MA_SUCCESS)
        return;

    deviceConfig = ma_device_config_init(ma_device_type_playback);
    deviceConfig.playback.format   = decoder.outputFormat;
    deviceConfig.playback.channels = decoder.outputChannels;
    deviceConfig.sampleRate        = decoder.outputSampleRate;
    deviceConfig.dataCallback      = miniAudioDataCallback;
    deviceConfig.pUserData         = &decoder;

    if (ma_device_init(NULL, &deviceConfig, &device) != MA_SUCCESS) {
        ma_decoder_uninit(&decoder);
        return;
    }

    if (ma_device_start(&device) != MA_SUCCESS) {
        ma_device_uninit(&device);
        ma_decoder_uninit(&decoder);
        return;
    }
}
