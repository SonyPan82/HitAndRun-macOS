#include <presentation/gui/guiscreenmessage.h>

#include <layer.h>
#include <page.h>
#include <screen.h>
#include <sprite.h>
#include <text.h>

#include <p3d/unicode.hpp>

int CGuiScreenMessage::s_ControllerDisconnectedPort = 0;
int CGuiScreenMessage::s_messageIndex = -1;
CGuiEntity* CGuiScreenMessage::s_pMessageCallback = nullptr;

CGuiScreenMessage::CGuiScreenMessage(Scrooby::Screen* screen, CGuiEntity* parent)
    : CGuiScreen(screen, parent, GUI_SCREEN_ID_GENERIC_MESSAGE), m_pMenu(nullptr),
      m_messageText(nullptr), m_messageIcon(nullptr), m_elapsedTime(0)
{
    m_originalStringBuffer[0] = 0;
    Scrooby::Page* page = screen ? screen->GetPage("Message") : nullptr;
    Scrooby::Layer* layer = page ? page->GetLayer("Foreground") : nullptr;
    if (layer) m_messageText = layer->GetText("Message");
    page = screen ? screen->GetPage("MessageBox") : nullptr;
    if (page) m_messageIcon = page->GetSprite("ErrorIcon");
}

CGuiScreenMessage::~CGuiScreenMessage() = default;

void CGuiScreenMessage::HandleMessage(eGuiMessage message, unsigned int param1, unsigned int param2)
{
    if (m_state == GUI_WINDOW_STATE_RUNNING && message == GUI_MSG_UPDATE)
    {
        m_elapsedTime += param1;
        if (s_pMessageCallback) s_pMessageCallback->HandleMessage(GUI_MSG_MESSAGE_UPDATE, param1);
    }
    CGuiScreen::HandleMessage(message, param1, param2);
}

void CGuiScreenMessage::Display(int messageIndex, CGuiEntity* callback)
{
    s_messageIndex = messageIndex;
    s_pMessageCallback = callback;
}

void CGuiScreenMessage::ConvertUnicodeToChar(char* string, P3D_UNICODE* unicode, int maxChars)
{
    if (!string || maxChars <= 0) return;
    int i = 0;
    if (unicode) for (; i < maxChars - 1 && unicode[i] != 0; ++i) string[i] = unicode[i] < 128 ? static_cast<char>(unicode[i]) : '?';
    string[i] = 0;
}

void CGuiScreenMessage::GetControllerDisconnectedMessage(int controllerId, char* string, int maxChars)
{
    s_ControllerDisconnectedPort = controllerId;
    if (!string || maxChars <= 0) return;
    const char* message = "Controller disconnected";
    int i = 0;
    for (; i < maxChars - 1 && message[i] != 0; ++i) string[i] = message[i];
    string[i] = 0;
}

void CGuiScreenMessage::FormatMessage(Scrooby::Text* text, UnicodeChar* original, int length)
{
    if (!text) return;
    UnicodeChar* buffer = text->GetStringBuffer();
    if (!buffer) return;
    if (original && length > 0)
    {
        int i = 0;
        for (; i < length - 1 && buffer[i] != 0; ++i) original[i] = buffer[i];
        original[i] = 0;
    }
    for (int i = 0; buffer[i] != 0; ++i)
        if (buffer[i] == static_cast<UnicodeChar>(0xa5)) { buffer[i] = static_cast<UnicodeChar>('1' + s_ControllerDisconnectedPort); break; }
}

void CGuiScreenMessage::InitIntro()
{
    if (m_messageText && s_messageIndex >= 0 && s_messageIndex < m_messageText->GetNumOfStrings())
    {
        m_messageText->SetIndex(s_messageIndex);
        FormatMessage(m_messageText, m_originalStringBuffer, sizeof(m_originalStringBuffer) / sizeof(m_originalStringBuffer[0]));
    }
    m_elapsedTime = 0;
}

void CGuiScreenMessage::InitRunning()
{
    if (s_pMessageCallback) s_pMessageCallback->HandleMessage(GUI_MSG_ON_DISPLAY_MESSAGE);
}

void CGuiScreenMessage::InitOutro() {}
