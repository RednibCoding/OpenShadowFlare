#include "runtime/platform/platform_text_input.hpp"

#include <psp2/common_dialog.h>
#include <psp2/display.h>
#include <psp2/ime_dialog.h>
#include <psp2/sysmodule.h>

#include <algorithm>
#include <array>
#include <string>

namespace osf::runtime {
namespace {

constexpr std::size_t kTextCapacity = 64;

void copyAsciiToUtf16(
    const std::string& source,
    SceWChar16* destination,
    std::size_t capacity) {
    const std::size_t count = std::min(
        source.size(), capacity > 0 ? capacity - 1 : 0);
    for (std::size_t index = 0; index < count; ++index) {
        destination[index] = static_cast<SceWChar16>(
            static_cast<unsigned char>(source[index]));
    }
    if (capacity > 0) {
        destination[count] = 0;
    }
}

std::string utf16ToAscii(
    const SceWChar16* source,
    int maxLength) {
    std::string result;
    for (int index = 0; index < maxLength && source[index] != 0; ++index) {
        const SceWChar16 codePoint = source[index];
        result.push_back(codePoint < 128
            ? static_cast<char>(codePoint)
            : '?');
    }
    return result;
}

}  // namespace

std::string openPlatformTextInput(
    const std::string& title,
    const std::string& initialText,
    int maxLength) {
    maxLength = std::clamp(
        maxLength, 1, static_cast<int>(kTextCapacity - 1));

    if (sceSysmoduleLoadModule(SCE_SYSMODULE_IME) < 0) {
        return {};
    }

    std::array<SceWChar16, kTextCapacity> titleUtf16{};
    std::array<SceWChar16, kTextCapacity> initialUtf16{};
    std::array<SceWChar16, kTextCapacity> resultUtf16{};
    copyAsciiToUtf16(title, titleUtf16.data(), titleUtf16.size());
    copyAsciiToUtf16(
        initialText, initialUtf16.data(), initialUtf16.size());

    SceImeDialogParam parameters{};
    sceImeDialogParamInit(&parameters);
    parameters.supportedLanguages =
        SCE_IME_LANGUAGE_ENGLISH | SCE_IME_LANGUAGE_PORTUGUESE_BR;
    parameters.type = SCE_IME_TYPE_BASIC_LATIN;
    parameters.option = SCE_IME_OPTION_NO_AUTO_CAPITALIZATION;
    parameters.dialogMode = SCE_IME_DIALOG_DIALOG_MODE_WITH_CANCEL;
    parameters.textBoxMode = SCE_IME_DIALOG_TEXTBOX_MODE_WITH_CLEAR;
    parameters.title = titleUtf16.data();
    parameters.maxTextLength = static_cast<SceUInt32>(maxLength);
    parameters.initialText = initialUtf16.data();
    parameters.inputTextBuffer = resultUtf16.data();

    if (sceImeDialogInit(&parameters) < 0) {
        sceSysmoduleUnloadModule(SCE_SYSMODULE_IME);
        return {};
    }

    for (;;) {
        const SceCommonDialogStatus status = sceImeDialogGetStatus();
        if (status == SCE_COMMON_DIALOG_STATUS_FINISHED) {
            break;
        }
        if (status == SCE_COMMON_DIALOG_STATUS_NONE) {
            sceSysmoduleUnloadModule(SCE_SYSMODULE_IME);
            return {};
        }
        sceDisplayWaitVblankStart();
    }

    SceImeDialogResult result{};
    const bool accepted = sceImeDialogGetResult(&result) >= 0 &&
        result.button == SCE_IME_DIALOG_BUTTON_ENTER;
    sceImeDialogTerm();
    sceSysmoduleUnloadModule(SCE_SYSMODULE_IME);
    return accepted ? utf16ToAscii(resultUtf16.data(), maxLength)
        : std::string{};
}

}  // namespace osf::runtime
