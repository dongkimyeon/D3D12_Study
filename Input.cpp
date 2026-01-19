#include "Input.h"

std::vector<Input::Key> Input::Keys = {};
std::wstring Input::mInputText;


static bool lastF1State = false;

int ASCII[(UINT)eKeyCode::End] =
{
    'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P',
    'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L',
    'Z', 'X', 'C', 'V', 'B', 'N', 'M',
    VK_LEFT, VK_RIGHT, VK_DOWN, VK_UP,
    VK_LBUTTON, VK_MBUTTON, VK_RBUTTON, VK_SPACE, VK_ESCAPE, VK_BACK,
    VK_SHIFT, VK_F1,
    '-',
    '='
};

void Input::ProcessChar(WPARAM wParam)
{
    if (wParam == VK_BACK)
    {
        if (!mInputText.empty())
        {
            mInputText.pop_back();
        }
    }
    else if (wParam >= 32 && wParam <= 126 && mInputText.length() < 20) 
    {
        mInputText += static_cast<wchar_t>(wParam);
    }
}

void Input::Initialize()
{
    createKeys();
    ClearInputText();
}

void Input::Update()
{
    updateKeys();
}

void Input::createKeys()
{
    for (size_t i = 0; i < (UINT)eKeyCode::End; i++)
    {
        Key key = {};
        key.bPressed = false;
        key.state = eKeyState::None;
        key.keyCode = (eKeyCode)i;

        Keys.push_back(key);
    }
}

void Input::updateKeys()
{
    std::for_each(Keys.begin(), Keys.end(),
        [](Key& key) -> void
        {
            updateKey(key);
        });
}

void Input::updateKey(Input::Key& key)
{
    if (GetFocus())
    {
        if (isKeyDown(key.keyCode))
            updateKeyDown(key);
        else
            updateKeyUp(key);


    }
    else
    {
        clearKeys();
    }
}

bool Input::isKeyDown(eKeyCode code)
{
    return GetAsyncKeyState(ASCII[(UINT)code]) & 0x8000;
}

void Input::updateKeyDown(Input::Key& key)
{
    if (key.bPressed == true)
        key.state = eKeyState::Pressed;
    else
        key.state = eKeyState::Down;

    key.bPressed = true;
}

void Input::updateKeyUp(Input::Key& key)
{
    if (key.bPressed == true)
        key.state = eKeyState::Up;
    else
        key.state = eKeyState::None;

    key.bPressed = false;
}



void Input::clearKeys()
{
    for (Key& key : Keys)
    {
        if (key.state == eKeyState::Down || key.state == eKeyState::Pressed)
            key.state = eKeyState::Up;
        else if (key.state == eKeyState::Up)
            key.state = eKeyState::None;

        key.bPressed = false;
    }
}