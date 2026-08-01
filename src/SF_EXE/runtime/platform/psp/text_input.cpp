#include "runtime/platform/platform_text_input.hpp"

#include <pspdisplay.h>
#include <pspgu.h>
#include <psputility.h>

#include <cstring>

namespace osf::runtime {
namespace {

unsigned int g_osk_display_list[64 * 1024] __attribute__((aligned(16)));

void asciiToUcs2(
    const std::string& source, unsigned short* out, int capacity) {
    int index = 0;
    for (; index < static_cast<int>(source.size()) && index < capacity - 1;
         ++index) {
        out[index] = static_cast<unsigned short>(
            static_cast<unsigned char>(source[index]));
    }
    out[index] = 0;
}

}  // namespace

std::string openPlatformTextInput(
    const std::string& title,
    const std::string& initial_text,
    int max_length) {
    if (max_length <= 0 || max_length > 126) {
        max_length = 32;
    }

    unsigned short description[128];
    unsigned short initial[128];
    unsigned short output[128];
    asciiToUcs2(title, description, 128);
    asciiToUcs2(initial_text, initial, 128);
    std::memset(output, 0, sizeof(output));

    SceUtilityOskData data;
    std::memset(&data, 0, sizeof(data));
    data.language = PSP_UTILITY_OSK_LANGUAGE_ENGLISH;
    data.lines = 1;
    data.unk_24 = 1;
    data.inputtype = PSP_UTILITY_OSK_INPUTTYPE_ALL;
    data.desc = description;
    data.intext = initial;
    data.outtextlength = max_length + 1;
    data.outtextlimit = max_length;
    data.outtext = output;

    SceUtilityOskParams params;
    std::memset(&params, 0, sizeof(params));
    params.base.size = sizeof(params);
    params.base.language = 1;
    params.base.buttonSwap = 1;
    params.base.graphicsThread = 17;
    params.base.accessThread = 19;
    params.base.fontThread = 18;
    params.base.soundThread = 16;
    params.datacount = 1;
    params.data = &data;

    if (sceUtilityOskInitStart(&params) < 0) {
        return std::string();
    }

    bool running = true;
    bool shutdown_requested = false;
    for (int guard = 0; running && guard < 7200; ++guard) {
        sceGuStart(GU_DIRECT, g_osk_display_list);
        sceGuClearColor(0xff000000);
        sceGuClear(GU_COLOR_BUFFER_BIT);
        sceGuFinish();
        sceGuSync(0, 0);

        switch (sceUtilityOskGetStatus()) {
        case PSP_UTILITY_DIALOG_VISIBLE:
            sceUtilityOskUpdate(1);
            break;
        case PSP_UTILITY_DIALOG_QUIT:
            if (!shutdown_requested) {
                sceUtilityOskShutdownStart();
                shutdown_requested = true;
            }
            break;
        case PSP_UTILITY_DIALOG_FINISHED:
        case PSP_UTILITY_DIALOG_NONE:
            running = false;
            break;
        default:
            break;
        }

        sceDisplayWaitVblankStart();
        sceGuSwapBuffers();
    }

    if (data.result == PSP_UTILITY_OSK_RESULT_CANCELLED) {
        return std::string();
    }

    std::string entered;
    for (int index = 0; index < max_length && output[index] != 0; ++index) {
        const unsigned short code = output[index];
        entered.push_back(code < 128 ? static_cast<char>(code) : '?');
    }
    return entered;
}

}  // namespace osf::runtime
