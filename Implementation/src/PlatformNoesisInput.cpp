#include "PlatformNoesisInput.hpp"

#include <NsGui/IView.h>

namespace NoesisDiligent
{
    namespace
    {
        std::uint32_t DecodeNextUtf8Codepoint(const char *&text)
        {
            const unsigned char lead = static_cast<unsigned char>(*text++);
            if ((lead & 0x80u) == 0)
            {
                return lead;
            }

            if ((lead & 0xE0u) == 0xC0u)
            {
                const unsigned char trail0 = static_cast<unsigned char>(*text++);
                return ((lead & 0x1Fu) << 6) | (trail0 & 0x3Fu);
            }

            if ((lead & 0xF0u) == 0xE0u)
            {
                const unsigned char trail0 = static_cast<unsigned char>(*text++);
                const unsigned char trail1 = static_cast<unsigned char>(*text++);
                return ((lead & 0x0Fu) << 12) | ((trail0 & 0x3Fu) << 6) | (trail1 & 0x3Fu);
            }

            const unsigned char trail0 = static_cast<unsigned char>(*text++);
            const unsigned char trail1 = static_cast<unsigned char>(*text++);
            const unsigned char trail2 = static_cast<unsigned char>(*text++);
            return ((lead & 0x07u) << 18) | ((trail0 & 0x3Fu) << 12) | ((trail1 & 0x3Fu) << 6) | (trail2 & 0x3Fu);
        }
    }

    Noesis::Key PlatformKeyCodeToNoesisKey(PlatformKeyCode key)
    {
        if (key >= PlatformKeyCode::A && key <= PlatformKeyCode::Z)
        {
            return static_cast<Noesis::Key>(Noesis::Key_A + (static_cast<int>(key) - static_cast<int>(PlatformKeyCode::A)));
        }

        if (key >= PlatformKeyCode::F1 && key <= PlatformKeyCode::F24)
        {
            return static_cast<Noesis::Key>(Noesis::Key_F1 + (static_cast<int>(key) - static_cast<int>(PlatformKeyCode::F1)));
        }

        switch (key)
        {
        case PlatformKeyCode::Num0: return Noesis::Key_D0;
        case PlatformKeyCode::Num1: return Noesis::Key_D1;
        case PlatformKeyCode::Num2: return Noesis::Key_D2;
        case PlatformKeyCode::Num3: return Noesis::Key_D3;
        case PlatformKeyCode::Num4: return Noesis::Key_D4;
        case PlatformKeyCode::Num5: return Noesis::Key_D5;
        case PlatformKeyCode::Num6: return Noesis::Key_D6;
        case PlatformKeyCode::Num7: return Noesis::Key_D7;
        case PlatformKeyCode::Num8: return Noesis::Key_D8;
        case PlatformKeyCode::Num9: return Noesis::Key_D9;

        case PlatformKeyCode::Return:
        case PlatformKeyCode::Return2:
        case PlatformKeyCode::KpEnter:
            return Noesis::Key_Return;
        case PlatformKeyCode::Escape: return Noesis::Key_Escape;
        case PlatformKeyCode::Backspace: return Noesis::Key_Back;
        case PlatformKeyCode::Tab: return Noesis::Key_Tab;
        case PlatformKeyCode::Space: return Noesis::Key_Space;
        case PlatformKeyCode::CapsLock: return Noesis::Key_CapsLock;
        case PlatformKeyCode::PrintScreen: return Noesis::Key_Print;
        case PlatformKeyCode::ScrollLock: return Noesis::Key_Scroll;
        case PlatformKeyCode::Pause: return Noesis::Key_Pause;
        case PlatformKeyCode::Insert: return Noesis::Key_Insert;
        case PlatformKeyCode::Home: return Noesis::Key_Home;
        case PlatformKeyCode::PageUp: return Noesis::Key_PageUp;
        case PlatformKeyCode::Delete: return Noesis::Key_Delete;
        case PlatformKeyCode::End: return Noesis::Key_End;
        case PlatformKeyCode::PageDown: return Noesis::Key_PageDown;
        case PlatformKeyCode::Right: return Noesis::Key_Right;
        case PlatformKeyCode::Left: return Noesis::Key_Left;
        case PlatformKeyCode::Down: return Noesis::Key_Down;
        case PlatformKeyCode::Up: return Noesis::Key_Up;
        case PlatformKeyCode::NumLockClear: return Noesis::Key_NumLock;

        case PlatformKeyCode::KpDivide: return Noesis::Key_Divide;
        case PlatformKeyCode::KpMultiply: return Noesis::Key_Multiply;
        case PlatformKeyCode::KpMinus: return Noesis::Key_Subtract;
        case PlatformKeyCode::KpPlus: return Noesis::Key_Add;
        case PlatformKeyCode::Kp1: return Noesis::Key_NumPad1;
        case PlatformKeyCode::Kp2: return Noesis::Key_NumPad2;
        case PlatformKeyCode::Kp3: return Noesis::Key_NumPad3;
        case PlatformKeyCode::Kp4: return Noesis::Key_NumPad4;
        case PlatformKeyCode::Kp5: return Noesis::Key_NumPad5;
        case PlatformKeyCode::Kp6: return Noesis::Key_NumPad6;
        case PlatformKeyCode::Kp7: return Noesis::Key_NumPad7;
        case PlatformKeyCode::Kp8: return Noesis::Key_NumPad8;
        case PlatformKeyCode::Kp9: return Noesis::Key_NumPad9;
        case PlatformKeyCode::Kp0: return Noesis::Key_NumPad0;
        case PlatformKeyCode::KpPeriod:
        case PlatformKeyCode::KpDecimal:
            return Noesis::Key_Decimal;
        case PlatformKeyCode::KpComma:
        case PlatformKeyCode::Separator:
            return Noesis::Key_Separator;

        case PlatformKeyCode::Application: return Noesis::Key_Apps;
        case PlatformKeyCode::Power:
        case PlatformKeyCode::Sleep:
            return Noesis::Key_Sleep;
        case PlatformKeyCode::Execute: return Noesis::Key_Execute;
        case PlatformKeyCode::Help: return Noesis::Key_Help;
        case PlatformKeyCode::Select: return Noesis::Key_Select;
        case PlatformKeyCode::Stop:
        case PlatformKeyCode::AcStop:
            return Noesis::Key_BrowserStop;
        case PlatformKeyCode::Mute: return Noesis::Key_VolumeMute;
        case PlatformKeyCode::VolumeUp: return Noesis::Key_VolumeUp;
        case PlatformKeyCode::VolumeDown: return Noesis::Key_VolumeDown;

        case PlatformKeyCode::LCtrl: return Noesis::Key_LeftCtrl;
        case PlatformKeyCode::LShift: return Noesis::Key_LeftShift;
        case PlatformKeyCode::LAlt: return Noesis::Key_LeftAlt;
        case PlatformKeyCode::LGui: return Noesis::Key_LWin;
        case PlatformKeyCode::RCtrl: return Noesis::Key_RightCtrl;
        case PlatformKeyCode::RShift: return Noesis::Key_RightShift;
        case PlatformKeyCode::RAlt: return Noesis::Key_RightAlt;
        case PlatformKeyCode::RGui: return Noesis::Key_RWin;

        case PlatformKeyCode::Semicolon: return Noesis::Key_Oem1;
        case PlatformKeyCode::Equals: return Noesis::Key_OemPlus;
        case PlatformKeyCode::Comma: return Noesis::Key_OemComma;
        case PlatformKeyCode::Minus: return Noesis::Key_OemMinus;
        case PlatformKeyCode::Period: return Noesis::Key_OemPeriod;
        case PlatformKeyCode::Slash: return Noesis::Key_Oem2;
        case PlatformKeyCode::Grave: return Noesis::Key_Oem3;
        case PlatformKeyCode::LeftBracket: return Noesis::Key_Oem4;
        case PlatformKeyCode::Backslash: return Noesis::Key_Oem5;
        case PlatformKeyCode::RightBracket: return Noesis::Key_Oem6;
        case PlatformKeyCode::Apostrophe: return Noesis::Key_Oem7;
        case PlatformKeyCode::NonUsBackslash: return Noesis::Key_Oem102;
        case PlatformKeyCode::International1: return Noesis::Key_AbntC1;
        case PlatformKeyCode::International2: return Noesis::Key_AbntC2;

        case PlatformKeyCode::Lang1: return Noesis::Key_HangulMode;
        case PlatformKeyCode::Lang2: return Noesis::Key_HanjaMode;
        case PlatformKeyCode::Lang3:
        case PlatformKeyCode::Lang4:
            return Noesis::Key_KanaMode;
        case PlatformKeyCode::Lang5:
        case PlatformKeyCode::Mode:
            return Noesis::Key_ImeModeChange;

        case PlatformKeyCode::Cancel: return Noesis::Key_Cancel;
        case PlatformKeyCode::Clear: return Noesis::Key_Clear;
        case PlatformKeyCode::Prior: return Noesis::Key_Prior;
        case PlatformKeyCode::ClearAgain: return Noesis::Key_OemClear;
        case PlatformKeyCode::CrSel: return Noesis::Key_CrSel;
        case PlatformKeyCode::ExSel: return Noesis::Key_ExSel;
        case PlatformKeyCode::AltErase: return Noesis::Key_EraseEof;
        case PlatformKeyCode::MediaPlay: return Noesis::Key_Play;
        case PlatformKeyCode::MediaPause:
        case PlatformKeyCode::MediaPlayPause:
            return Noesis::Key_MediaPlayPause;
        case PlatformKeyCode::MediaNextTrack: return Noesis::Key_MediaNextTrack;
        case PlatformKeyCode::MediaPreviousTrack: return Noesis::Key_MediaPreviousTrack;
        case PlatformKeyCode::MediaStop: return Noesis::Key_MediaStop;
        case PlatformKeyCode::MediaSelect: return Noesis::Key_SelectMedia;
        case PlatformKeyCode::AcBack: return Noesis::Key_BrowserBack;
        case PlatformKeyCode::AcForward: return Noesis::Key_BrowserForward;
        case PlatformKeyCode::AcRefresh: return Noesis::Key_BrowserRefresh;
        case PlatformKeyCode::AcSearch: return Noesis::Key_BrowserSearch;
        case PlatformKeyCode::AcHome: return Noesis::Key_BrowserHome;
        case PlatformKeyCode::AcBookmarks: return Noesis::Key_BrowserFavorites;
        case PlatformKeyCode::AcPrint: return Noesis::Key_Print;

        default:
            return Noesis::Key_None;
        }
    }

    Noesis::MouseButton PlatformMouseButtonToNoesisButton(PlatformMouseButton button)
    {
        switch (button)
        {
        case PlatformMouseButton::Left:
            return Noesis::MouseButton_Left;
        case PlatformMouseButton::Middle:
            return Noesis::MouseButton_Middle;
        case PlatformMouseButton::Right:
            return Noesis::MouseButton_Right;
        case PlatformMouseButton::X1:
            return Noesis::MouseButton_XButton1;
        case PlatformMouseButton::X2:
            return Noesis::MouseButton_XButton2;
        default:
            return Noesis::MouseButton_Left;
        }
    }

    void SendUtf8TextToNoesis(Noesis::IView& view, const char* text)
    {
        if (text == nullptr)
        {
            return;
        }

        while (*text != '\0')
        {
            view.Char(DecodeNextUtf8Codepoint(text));
        }
    }
}