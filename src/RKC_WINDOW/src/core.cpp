/**
 * RKC_WINDOW - Window management class
 * 
 * Wrapper for window operations. Most functions still forward to original DLL.
 * USED BY: ShadowFlare.exe (constructor and destructor only)
 */

#include <windows.h>
#include <cstdint>
#include <cstring>

struct RKC_WINDOW {
    std::uint8_t bytes[0x550];
};

static_assert(sizeof(RKC_WINDOW) == 0x550, "RKC_WINDOW ABI size mismatch");

template <typename T>
static T& Field(RKC_WINDOW* self, std::size_t offset)
{
    return *reinterpret_cast<T*>(self->bytes + offset);
}

static constexpr long kUseScrollTrackPosition = static_cast<long>(0x8fffffff);

extern "C" {
    /**
     * Constructor - initialize all members to zero
     * USED BY: ShadowFlare.exe
     */
    RKC_WINDOW* __thiscall RKC_Window_constructor(RKC_WINDOW* self)
    {
        Field<std::uint32_t>(self, 0x000) = 0;
        Field<std::uint32_t>(self, 0x004) = 0;
        self->bytes[0x008] = 0;
        self->bytes[0x108] = 0;
        std::memset(self->bytes + 0x508, 0, 10 * sizeof(std::uint32_t));
        std::memset(self->bytes + 0x530, 0, 8 * sizeof(std::uint32_t));
        return self;
    }

    /**
     * Destructor - empty
     * USED BY: ShadowFlare.exe
     */
    void __thiscall RKC_Window_deconstructor(RKC_WINDOW* self)
    {
        return;
    }

    /**
     * Assignment operator - shallow copy
     * NOT REFERENCED - not imported by any module
     */
    RKC_WINDOW& __thiscall EqualsOperator(RKC_WINDOW* self, const RKC_WINDOW& other)
    {
        std::memcpy(self, &other, sizeof(*self));
        return *self;
    }

    /**
     * Horizontal scroll handler
     * NOT REFERENCED - stub only, not imported by any module
     */
    void __thiscall HScroll(RKC_WINDOW* self, std::uint32_t scrollCode, long position)
    {
        RECT client{};
        GetClientRect(Field<HWND>(self, 0x000), &client);

        long& x = Field<long>(self, 0x540);
        switch (scrollCode & 0xffff) {
        case SB_LINEUP:
            x -= Field<long>(self, 0x544);
            break;
        case SB_LINEDOWN:
            x += Field<long>(self, 0x544);
            break;
        case SB_PAGEUP:
            x -= client.right;
            break;
        case SB_PAGEDOWN:
            x += client.right;
            break;
        case SB_THUMBPOSITION:
        case SB_THUMBTRACK:
            if (position == kUseScrollTrackPosition) {
                SCROLLINFO track{};
                track.cbSize = sizeof(track);
                track.fMask = SIF_TRACKPOS | SIF_POS;
                GetScrollInfo(Field<HWND>(self, 0x000), SB_HORZ, &track);
                position = track.nTrackPos;
            }
            x = position;
            break;
        default:
            break;
        }

        if (x < 0)
            x = 0;
        const long maximum = Field<long>(self, 0x548) - client.right;
        if (maximum < x)
            x = maximum;

        SCROLLINFO info{};
        info.cbSize = sizeof(info);
        info.fMask = SIF_POS;
        info.nPos = x;
        SetScrollInfo(Field<HWND>(self, 0x000), SB_HORZ, &info, TRUE);
    }

    /**
     * Vertical scroll handler
     * NOT REFERENCED - stub only, not imported by any module
     */
    void __thiscall VScroll(RKC_WINDOW* self, std::uint32_t scrollCode, long position)
    {
        RECT client{};
        GetClientRect(Field<HWND>(self, 0x000), &client);

        long& y = Field<long>(self, 0x538);
        switch (scrollCode & 0xffff) {
        case SB_LINEUP:
            y -= Field<long>(self, 0x53c);
            break;
        case SB_LINEDOWN:
            y += Field<long>(self, 0x53c);
            break;
        case SB_PAGEUP:
            y -= client.bottom;
            break;
        case SB_PAGEDOWN:
            y += client.bottom;
            break;
        case SB_THUMBPOSITION:
        case SB_THUMBTRACK:
            if (position == kUseScrollTrackPosition) {
                SCROLLINFO track{};
                track.cbSize = sizeof(track);
                track.fMask = SIF_TRACKPOS | SIF_POS;
                GetScrollInfo(Field<HWND>(self, 0x000), SB_VERT, &track);
                position = track.nTrackPos;
            }
            y = position;
            break;
        default:
            break;
        }

        if (y < 0)
            y = 0;
        const long maximum = Field<long>(self, 0x54c) - client.bottom;
        if (maximum < y)
            y = maximum;

        SCROLLINFO info{};
        info.cbSize = sizeof(info);
        info.fMask = SIF_POS;
        info.nPos = y;
        SetScrollInfo(Field<HWND>(self, 0x000), SB_VERT, &info, TRUE);
    }

    /**
     * Resize handler - updates scroll bars based on client rect
     * NOT REFERENCED - stub only, not imported by any module
     */
    void __thiscall Resize(RKC_WINDOW* self)
    {
        RECT client{};
        SCROLLINFO info{};
        info.cbSize = sizeof(info);
        info.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
        info.nMin = 0;

        GetClientRect(Field<HWND>(self, 0x000), &client);
        const long contentWidth = Field<long>(self, 0x548);
        const long contentHeight = Field<long>(self, 0x54c);
        long& x = Field<long>(self, 0x540);
        long& y = Field<long>(self, 0x538);

        if (contentWidth <= client.right && contentHeight <= client.bottom) {
            info.nMax = 0;
            info.nPage = 0;
            info.nPos = 0;
            x = 0;
            y = 0;
            SetScrollInfo(Field<HWND>(self, 0x000), SB_VERT, &info, TRUE);
            SetScrollInfo(Field<HWND>(self, 0x000), SB_HORZ, &info, TRUE);
            return;
        }

        GetClientRect(Field<HWND>(self, 0x000), &client);
        const long maxY = contentHeight - client.bottom;
        if (maxY <= 0) {
            info.nMax = 0;
            info.nPage = 0;
            info.nPos = 0;
            y = 0;
        } else {
            info.nMax = contentHeight - 1;
            info.nPage = client.bottom;
            info.nPos = y;
            if (y < 0)
                y = 0;
            if (maxY < y)
                y = maxY;
        }
        SetScrollInfo(Field<HWND>(self, 0x000), SB_VERT, &info, TRUE);

        GetClientRect(Field<HWND>(self, 0x000), &client);
        const long maxX = contentWidth - client.right;
        if (maxX <= 0) {
            info.nMax = 0;
            info.nPage = 0;
            info.nPos = 0;
            x = 0;
        } else {
            info.nMax = contentWidth - 1;
            info.nPage = client.right;
            info.nPos = x;
            if (x < 0)
                x = 0;
            if (maxX < x)
                x = maxX;
        }
        SetScrollInfo(Field<HWND>(self, 0x000), SB_HORZ, &info, TRUE);
    }

    /**
     * Show/hide window
     * NOT REFERENCED - stub only, not imported by any module
     */
    void __thiscall Show(RKC_WINDOW* self, int showCmd)
    {
        if (showCmd == 1) {
            SetWindowPos(
                Field<HWND>(self, 0x000),
                nullptr,
                0,
                0,
                0,
                0,
                SWP_SHOWWINDOW | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER);
            if (Field<long>(self, 0x534) == 1) {
                Resize(self);
                Field<long>(self, 0x530) = 1;
                return;
            }
        } else if (showCmd == 0) {
            SetWindowPos(
                Field<HWND>(self, 0x000),
                nullptr,
                0,
                0,
                0,
                0,
                SWP_HIDEWINDOW | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER);
        } else {
            return;
        }
        Field<long>(self, 0x530) = showCmd;
    }
}


bool __stdcall DllMain(LPVOID, std::uint32_t call_reason, LPVOID)
{
    return true;
}
