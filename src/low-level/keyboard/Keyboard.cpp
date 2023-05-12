#include <vtconsole/vtconsole.h>
#include "Keyboard.hpp"
#include "debug.hpp"
#include "layouts/QWERTYKeyboard.hpp"
#include "filesystems/VFS.hpp"
#include "input/InputSystem.hpp"

#define LAYOUT_EXT "layout"

struct KeyInfo
{
    const char *text;
    int key;
};

#define FILLKEY(name) \
    { .text=#name, .key=TOAST_##name }

KeyInfo keys[] =
{
    FILLKEY(KEY_A),
    FILLKEY(KEY_B),
    FILLKEY(KEY_C),
    FILLKEY(KEY_D),
    FILLKEY(KEY_E),
    FILLKEY(KEY_F),
    FILLKEY(KEY_G),
    FILLKEY(KEY_H),
    FILLKEY(KEY_I),
    FILLKEY(KEY_J),
    FILLKEY(KEY_K),
    FILLKEY(KEY_L),
    FILLKEY(KEY_M),
    FILLKEY(KEY_N),
    FILLKEY(KEY_O),
    FILLKEY(KEY_P),
    FILLKEY(KEY_Q),
    FILLKEY(KEY_R),
    FILLKEY(KEY_S),
    FILLKEY(KEY_T),
    FILLKEY(KEY_U),
    FILLKEY(KEY_V),
    FILLKEY(KEY_Y),
    FILLKEY(KEY_Z),
    FILLKEY(KEY_1),
    FILLKEY(KEY_2),
    FILLKEY(KEY_3),
    FILLKEY(KEY_4),
    FILLKEY(KEY_5),
    FILLKEY(KEY_6),
    FILLKEY(KEY_7),
    FILLKEY(KEY_8),
    FILLKEY(KEY_9),
    FILLKEY(KEY_0),
    FILLKEY(KEY_LEFT_SHIFT),
    FILLKEY(KEY_RIGHT_SHIFT),
    FILLKEY(KEY_LEFT_CONTROL),
    FILLKEY(KEY_RIGHT_CONTROL),
    FILLKEY(KEY_LEFT_ARROW),
    FILLKEY(KEY_RIGHT_ARROW),
    FILLKEY(KEY_UP_ARROW),
    FILLKEY(KEY_DOWN_ARROW),
    FILLKEY(KEY_SPACE),
    FILLKEY(KEY_BACKSPACE),
    FILLKEY(KEY_RETURN),
    FILLKEY(KEY_ESCAPE),
    FILLKEY(KEY_F1),
    FILLKEY(KEY_F2),
    FILLKEY(KEY_F3),
    FILLKEY(KEY_F4),
    FILLKEY(KEY_F5),
    FILLKEY(KEY_F6),
    FILLKEY(KEY_F7),
    FILLKEY(KEY_F8),
    FILLKEY(KEY_F9),
    FILLKEY(KEY_F10),
    FILLKEY(KEY_F11),
    FILLKEY(KEY_F12),
    FILLKEY(KEY_TAB),
    FILLKEY(KEY_CAPS_LOCK),
};

bool isLeftShiftPressed;
bool isRightShiftPressed;

bool gotKeyboardInput = false;
char keyboardInput;

struct KeyboardLayoutItem
{
    uint32_t scancode;
    string modifier;
    string value;
    uint8_t key;
};

struct KeyboardLayout
{
public:
    vector<KeyboardLayoutItem> items;

    KeyboardLayoutItem *lshiftKey;
    KeyboardLayoutItem *rshiftKey;
    KeyboardLayoutItem *spaceKey;
    KeyboardLayoutItem *returnKey;
    KeyboardLayoutItem *backspaceKey;

    vector<KeyboardLayoutItem *> oemKeys;

    string name;

    void Clear()
    {
        oemKeys.clear();
        items.clear();

        lshiftKey = rshiftKey = spaceKey = returnKey = backspaceKey = NULL;
    }

    void UpdateModifiers()
    {
        for(auto &entry : items)
        {
            if(entry.value == "SPACE")
            {
                spaceKey = &entry;
            }
            else if(entry.value == "LSHIFT")
            {
                lshiftKey = &entry;
            }
            else if(entry.value == "RSHIFT")
            {
                rshiftKey = &entry;
            }
            else if(entry.value == "RETURN")
            {
                returnKey = &entry;
            }
            else if(entry.value == "BACKSPACE")
            {
                backspaceKey = &entry;
            }
            else if(strstr(entry.value.data(), "OEM") == entry.value.data())
            {
                oemKeys.push_back(&entry);
            }
        }
    }
};

KeyboardLayout *currentLayout = NULL;

uint8_t KeyFromString(const string &key)
{
    for(uint64_t i = 0; i < sizeof(keys) / sizeof(keys[0]); i++)
    {
        if(key == keys[i].text)
        {
            return keys[i].key;
        }
    }

    return 0;
}

bool LoadLayout(const string &name, int *error)
{
    DEBUG_OUT("Loading keyboard layout \"%s\"", name.data());

    auto path = string("/system/kbd/layouts/") + name + "." + LAYOUT_EXT;

    auto handle = vfs->OpenFile(path.data(), 0, NULL, error);

    if(vfs->FileType(handle) == FILE_HANDLE_FILE || *error != 0)
    {
        auto length = vfs->FileLength(handle);

        auto buffer = new char[length + 1];

        buffer[length] = '\0';

        if(vfs->ReadFile(handle, buffer, length, error) != length || *error != 0)
        {
            delete [] buffer;
            
            vfs->CloseFile(handle);

            return false;
        }

        vfs->CloseFile(handle);

        KeyboardLayout *layout = new KeyboardLayout();

        layout->name = name;

        size_t cursor = 0;

        string content = buffer;

        delete [] buffer;

        frg::string_view view(content);

        bool error = false;

        for(;;)
        {
            size_t position = view.find_first('\n', cursor);
            string line;
            bool needsQuit = false;

            if(position == (size_t)-1 && cursor + 1 < content.size())
            {
                line = string(view.sub_string(cursor, view.size() - cursor - 1));

                needsQuit = true;
            }
            else
            {
                line = string(view.sub_string(cursor, position - cursor));
            }

            cursor = position + 1;

            frg::string_view lineView(line);

            position = lineView.find_first('=');

            if(position != (size_t)-1)
            {
                auto left = string(lineView.sub_string(0, position));
                auto right = lineView.sub_string(position + 1, lineView.size() - position - 1);

                auto leftView = frg::string_view(left);

                auto mod = leftView.find_first(':');
                string modifier;

                if(mod != (size_t)-1)
                {
                    modifier = string(leftView.sub_string(mod + 1, left.size() - mod - 1));

                    left = string(leftView.sub_string(0, mod));

                    leftView = frg::string_view(left);
                }

                auto scancode = strtol(left.data(), NULL, 16);

                auto k = right.find_first(':');

                if(k == (size_t)-1)
                {
                    printf("At line '%s': Missing key at right side\n", line.data());

                    error = true;

                    if(needsQuit)
                    {
                        break;
                    }

                    continue;
                }

                auto key = right.sub_string(k + 1, right.size() - k - 1);

                right = right.sub_string(0, k);

                KeyboardLayoutItem item;

                item.scancode = scancode;
                item.modifier = modifier;
                item.value = string(right);
                item.key = KeyFromString(string(key));

                layout->items.push_back(item);
            }

            if(cursor >= view.size() || needsQuit)
            {
                break;
            }
        }

        if(error)
        {
            delete layout;

            return false;
        }

        layout->UpdateModifiers();

        if(currentLayout != NULL)
        {
            delete currentLayout;
        }

        currentLayout = layout;
    }
    else
    {
        DEBUG_OUT("Failed to open file for keyboard layout located at \"%s\"", path.data());

        vfs->CloseFile(handle);

        return false;
    }

    return true;
}

extern "C" void InitializeKeyboard()
{
    DEBUG_OUT("%s", "Initializing keyboard");

#if USE_INPUT_SYSTEM
    int error = 0;

    auto handle = vfs->OpenFile("/system/kbd/active", 0, NULL, &error);

    if(vfs->FileType(handle) == FILE_HANDLE_FILE && error == 0)
    {
        auto length = vfs->FileLength(handle);

        char *buffer = new char[length + 1];

        buffer[length] = '\0';

        if(vfs->ReadFile(handle, buffer, length, &error) != length || error != 0)
        {
            delete [] buffer;

            vfs->CloseFile(handle);

            return;
        }

        vfs->CloseFile(handle);

        if(!LoadLayout(buffer, &error) || error != 0)
        {
            delete [] buffer;

            DEBUG_OUT("%s", "Failed to load keyboard layout");

            return;
        }
        else
        {
            delete [] buffer;

            DEBUG_OUT("%s", "Successfully loaded layout!");
        }
    }

#endif
}

extern "C" bool SetKeyboardLayout(const char *name, int *error)
{
    return LoadLayout(name, error);
}

extern "C" const char *GetKeyboardLayoutName()
{
    return currentLayout != NULL ? currentLayout->name.data() : "";
}

extern "C" bool GotKeyboardInput()
{
    return gotKeyboardInput;
}

extern "C" char KeyboardInput()
{
    gotKeyboardInput = false;
    
    return keyboardInput;
}

InputEvent MakeKeyboardEvent(uint8_t scancode, uint16_t character, uint8_t key, bool leftShift, bool rightShift)
{
    InputEvent event = {.type = scancode >= 0x80 ? TOAST_INPUT_EVENT_KEYUP : TOAST_INPUT_EVENT_KEYDOWN };

    event.keyEvent.key = key;
    event.keyEvent.character = character;
    event.keyEvent.modifiers = (leftShift ? TOAST_INPUT_MODIFIER_LSHIFT : 0) | (rightShift ? TOAST_INPUT_MODIFIER_RSHIFT : 0);

    return event;
}

extern "C" void HandleKeyboardKeyPress(uint8_t scancode)
{
#if USE_INPUT_SYSTEM
    if(currentLayout != NULL && currentLayout->items.size() != 0)
    {
        if(currentLayout->lshiftKey != NULL)
        {
            if(scancode == currentLayout->lshiftKey->scancode)
            {
                isLeftShiftPressed = true;

                globalInputSystem->AddEvent(MakeKeyboardEvent(scancode, 0, TOAST_KEY_LEFT_SHIFT, isLeftShiftPressed, isRightShiftPressed));

                return;
            }
            else if(scancode == currentLayout->lshiftKey->scancode + 0x80)
            {
                isLeftShiftPressed = false;

                globalInputSystem->AddEvent(MakeKeyboardEvent(scancode, 0, TOAST_KEY_LEFT_SHIFT, isLeftShiftPressed, isRightShiftPressed));

                return;
            }
        }
        
        if(currentLayout->rshiftKey != NULL)
        {
            if(scancode == currentLayout->rshiftKey->scancode)
            {
                isRightShiftPressed = true;

                globalInputSystem->AddEvent(MakeKeyboardEvent(scancode, 0, TOAST_KEY_RIGHT_SHIFT, isLeftShiftPressed, isRightShiftPressed));

                return;
            }
            else if(scancode == currentLayout->rshiftKey->scancode + 0x80)
            {
                isRightShiftPressed = false;

                globalInputSystem->AddEvent(MakeKeyboardEvent(scancode, 0, TOAST_KEY_RIGHT_SHIFT, isLeftShiftPressed, isRightShiftPressed));

                return;
            }
        }
        
        if(currentLayout->returnKey != NULL && scancode == currentLayout->returnKey->scancode)
        {
            globalInputSystem->AddEvent(MakeKeyboardEvent(scancode, '\n', TOAST_KEY_RETURN, isLeftShiftPressed, isRightShiftPressed));

            return;
        }
        
        if(currentLayout->spaceKey != NULL && scancode == currentLayout->spaceKey->scancode)
        {
            globalInputSystem->AddEvent(MakeKeyboardEvent(scancode, ' ', TOAST_KEY_SPACE, isLeftShiftPressed, isRightShiftPressed));

            return;
        }
        
        if(currentLayout->backspaceKey != NULL && scancode == currentLayout->backspaceKey->scancode)
        {
            globalInputSystem->AddEvent(MakeKeyboardEvent(scancode, '\b', TOAST_KEY_BACKSPACE, isLeftShiftPressed, isRightShiftPressed));

            return;
        }

        for(auto &entry : currentLayout->items)
        {
            if(entry.scancode == scancode || entry.scancode + 0x80 == scancode)
            {
                if(entry.modifier.size() > 0)
                {
                    if(entry.modifier == "SHIFT" && (isLeftShiftPressed || isRightShiftPressed))
                    {
                        globalInputSystem->AddEvent(MakeKeyboardEvent(scancode, entry.value[0], entry.key, isLeftShiftPressed, isRightShiftPressed));

                        return;
                    }

                    continue;
                }

                globalInputSystem->AddEvent(MakeKeyboardEvent(scancode, entry.value[0], entry.key, isLeftShiftPressed, isRightShiftPressed));

                break;
            }
        }
    }
#else

    switch (scancode)
    {
        case Enter:
            gotKeyboardInput = true;
            keyboardInput = '\n';

            return;

        case Spacebar:
            gotKeyboardInput = true;
            keyboardInput = ' ';

            return;

        case BackSpace:
            gotKeyboardInput = true;
            keyboardInput = '\b';

            return;

        case LeftShift:
            isLeftShiftPressed = true;

            return;

        case LeftShift + 0x80:
            isLeftShiftPressed = false;

            return;

        case RightShift:
            isRightShiftPressed = true;

            return;

        case RightShift + 0x80:
            isRightShiftPressed = false;

            return;
    }

    char ascii = QWERTYKeyboard::Translate(scancode, isLeftShiftPressed | isRightShiftPressed);

    if (ascii != 0)
    {
        gotKeyboardInput = true;
        keyboardInput = ascii;
    }
#endif
}
