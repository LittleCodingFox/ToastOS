#pragma once

#include "frgdef.hpp"

struct App
{
    string name;
    string icon;
    string exe;
    int iconTexture;
    int iconWidth;
    int iconHeight;

    App() : iconTexture(0), iconWidth(0), iconHeight(0) {}
};

void EnumerateApps();

extern box<vector<App>> apps;
