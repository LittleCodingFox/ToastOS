#include <vtconsole/vtconsole.h>
#include "Keyboard.hpp"
#include "debug.hpp"
#include "layouts/QWERTYKeyboard.hpp"
#include "filesystems/VFS.hpp"

using namespace FileSystem;

bool isLeftShiftPressed;
bool isRightShiftPressed;

bool gotKeyboardInput = false;
char keyboardInput;

struct KeyboardLayoutItem
{
    int scancode;
    string value;
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

box<KeyboardLayout> currentLayout;

bool LoadLayout(const string &name)
{
    DEBUG_OUT("Loading keyboard layout \"%s\"", name.data());

    auto path = string("/system/kbd/layouts/") + name + ".ini";

    auto handle = vfs->OpenFile(path.data(), 0, NULL);

    if(vfs->FileType(handle) == FILE_HANDLE_FILE)
    {
        auto length = vfs->FileLength(handle);

        auto buffer = new char[length] + 1;

        buffer[length] = '\0';

        if(vfs->ReadFile(handle, buffer, length) != length)
        {
            delete [] buffer;
            
            vfs->CloseFile(handle);

            return false;
        }

        vfs->CloseFile(handle);

        if(!currentLayout.valid())
        {
            currentLayout.initialize();
        }
        else
        {
            currentLayout->Clear();
        }

        currentLayout->name = name;

        size_t cursor = 0;

        string content = buffer;

        delete [] buffer;

        frg::string_view view(content);

        for(;;)
        {
            size_t position = view.find_first('\n', cursor);
            string line;
            bool needsQuit = false;

            if(position == (size_t)-1 && cursor + 1 < content.size())
            {
                line = view.sub_string(cursor, view.size() - cursor - 1);

                needsQuit = true;
            }
            else
            {
                line = view.sub_string(cursor, position - cursor);
            }

            cursor = position + 1;

            frg::string_view lineView(line);

            position = lineView.find_first('=');

            if(position != (size_t)-1)
            {
                auto left = lineView.sub_string(0, position);
                auto right = lineView.sub_string(position + 1, lineView.size() - position - 1);

                auto scancode = strtol(left.data(), NULL, 16);

                KeyboardLayoutItem item;

                item.scancode = scancode;
                item.value = right;

                currentLayout->items.push_back(item);
            }

            if(cursor >= view.size() || needsQuit)
            {
                break;
            }
        }

        currentLayout->UpdateModifiers();
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
    DEBUG_OUT("Initializing keyboard", 0);

    auto handle = vfs->OpenFile("/system/kbd/active", 0, NULL);

    if(vfs->FileType(handle) == FILE_HANDLE_FILE)
    {
        auto length = vfs->FileLength(handle);

        char *buffer = new char[length + 1];

        buffer[length] = '\0';

        if(vfs->ReadFile(handle, buffer, length) != length)
        {
            delete [] buffer;

            vfs->CloseFile(handle);

            return;
        }

        vfs->CloseFile(handle);

        if(!LoadLayout(buffer))
        {
            delete [] buffer;

            DEBUG_OUT("Failed to load keyboard layout", 0);

            return;
        }
        else
        {
            delete [] buffer;

            DEBUG_OUT("Successfully loaded layout!", 0);
        }
    }
}

extern "C" const char *GetKeyboardLayoutName()
{
    return currentLayout.valid() ? currentLayout->name.data() : "";
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

extern "C" void HandleKeyboardKeyPress(uint8_t scancode)
{
    if(currentLayout.valid() && currentLayout->items.size() != 0)
    {
        if(currentLayout->lshiftKey != NULL)
        {
            if(scancode == currentLayout->lshiftKey->scancode)
            {
                isLeftShiftPressed = true;

                return;
            }
            else if(scancode == currentLayout->lshiftKey->scancode + 0x80)
            {
                isLeftShiftPressed = false;

                return;
            }
        }
        
        if(currentLayout->rshiftKey != NULL)
        {
            if(scancode == currentLayout->rshiftKey->scancode)
            {
                isRightShiftPressed = true;

                return;
            }
            else if(scancode == currentLayout->rshiftKey->scancode + 0x80)
            {
                isRightShiftPressed = false;

               return;
            }
        }
        
        if(currentLayout->returnKey != NULL && scancode == currentLayout->returnKey->scancode)
        {
            gotKeyboardInput = true;
            keyboardInput = '\n';

            return;
        }
        
        if(currentLayout->spaceKey != NULL && scancode == currentLayout->spaceKey->scancode)
        {
            gotKeyboardInput = true;
            keyboardInput = ' ';

            return;
        }
        
        if(currentLayout->backspaceKey != NULL && scancode == currentLayout->backspaceKey->scancode)
        {
            gotKeyboardInput = true;
            keyboardInput = '\b';

            return;
        }

        for(auto &entry : currentLayout->items)
        {
            if(entry.scancode == scancode)
            {
                if(entry.value.size() == 1)
                {
                    gotKeyboardInput = true;
                    keyboardInput = entry.value[0];

                    return;
                }
            }
        }

        return;
    }

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
    }

    char ascii = QWERTYKeyboard::Translate(scancode, isLeftShiftPressed | isRightShiftPressed);

    if (ascii != 0)
    {
        gotKeyboardInput = true;
        keyboardInput = ascii;
    }
}
