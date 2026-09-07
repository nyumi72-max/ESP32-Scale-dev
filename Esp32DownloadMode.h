#ifndef ESP32DOWNLOADMODE_H
#define ESP32DOWNLOADMODE_H

#include <Arduino.h>
#include <esp_system.h>
#include <soc/soc.h>
#include <soc/rtc_cntl_reg.h>

// ESP32のROM UARTダウンロードモードを強制して再起動する。
// esp_restart()で戻らないため戻り値はない。
// 単一の関数だけなので.cppは作らずヘッダーオンリー(inline)にしている。
inline void esp32EnterDownloadMode()
{
    delay(100);

    REG_WRITE(RTC_CNTL_OPTION1_REG, RTC_CNTL_FORCE_DOWNLOAD_BOOT);

    esp_restart();

    while (true)
    {
        // 到達しない
    }
}

#endif
