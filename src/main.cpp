#include "common.h"
#include "mem.h"
#include "game.h"
#include "hook.h"

static DWORD WINAPI ScanThread(LPVOID) {
    unsigned faults = 0;
    while (cfg::running.load()) {
        __try { game::ScanLoop(); }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            game::g_controllerProbe.exceptionCode.store(GetExceptionCode());
            game::g_controllerProbe.scanFaults.fetch_add(1);
            game::g_sys.localCtrl = 0;
            game::g_sys.ready = false;
            ++faults;
        }
        if (!cfg::running.load()) break;
        // Bounded recovery; repeated faults remain visible, not a silent dead thread.
        if (faults >= 3) break;
        Sleep(500);
    }
    return 0;
}

static DWORD WINAPI InitThread(LPVOID) {
    while (cfg::running.load()) {
        game::TryInit();
        Sleep(game::g_sys.ready ? 1000 : 400);
    }
    return 0;
}
static DWORD WINAPI MainThread(LPVOID) {
    __try {
        bool haveClient = false;
        for (int i = 0; i < 600 && cfg::running.load(); ++i) {
            if (mem::ModuleBase("client.dll")) {
                haveClient = true;
                break;
            }
            Sleep(200);
        }
        if (!haveClient) {
            while (cfg::running.load()) Sleep(500);
            return 0;
        }

        InstallHooks();

        HANDLE scan = CreateThread(nullptr, 0, ScanThread, nullptr, 0, nullptr);
        if (scan) CloseHandle(scan);

        HANDLE init = CreateThread(nullptr, 0, InitThread, nullptr, 0, nullptr);
        if (init) CloseHandle(init);

        while (cfg::running.load()) Sleep(500);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
    }
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        HANDLE t = CreateThread(nullptr, 0, MainThread, hModule, 0, nullptr);
        if (t) CloseHandle(t);
    }
    return TRUE;
}
