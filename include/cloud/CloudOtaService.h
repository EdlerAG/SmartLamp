#pragma once
#include <Arduino.h>
#include "core/AppState.h"
#include "cloud/StandaloneOta.h"

/**
 * ============================================================================
 * CloudOtaService (Bridge to StandaloneOta v3.8)
 * Integrates firmware updates with AppState, display progress, and FreeRTOS.
 * ============================================================================
 */

class CloudOtaService {
private:
    struct OtaStateGuard {
        AppState& st;
        OtaStateGuard(AppState& s) : st(s) {
            st.ota_in_progress = true;
            st.ota_progress    = 0;
            st.tls_in_flight   = true;
            WiFi.setSleep(false);
            st.registerActivity();
        }
        ~OtaStateGuard() {
            st.ota_in_progress = false;
            st.tls_in_flight   = false;
            st.registerNetActivity();
            st.registerActivity();
        }
    };

    static AppState*& getStatePtr() {
        static AppState* ptr = nullptr;
        return ptr;
    }

    static void onProgress(uint8_t percent, const char* /*status_msg*/) {
        AppState* st = getStatePtr();
        if (st) {
            st->ota_progress = percent;
            st->registerActivity();
        }
    }

public:
    static String getPartitionInfo() {
        return StandaloneOta::getPartitionInfo();
    }

    static bool rollbackToPrevious(String& err_msg) {
        return StandaloneOta::rollback(&err_msg);
    }

    static bool flashFromUrl(AppState& state, const String& url, String& err_msg, const char* expected_sha256 = nullptr) {
        OtaStateGuard guard(state);
        getStatePtr() = &state;

        bool ok = StandaloneOta::flashFromUrl(
            url.c_str(),
            expected_sha256,
            onProgress,
            &err_msg,
            false // Не перезавантажуємося одразу всередині, щоб викликати delay та ESP.restart() контрольовано
        );

        getStatePtr() = nullptr;
        if (ok) {
            state.ota_progress = 100;
            state.registerActivity();
        }
        return ok;
    }
};
