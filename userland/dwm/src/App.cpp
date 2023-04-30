#include "App.hpp"
#include <sys/types.h>
#include <dirent.h>

box<vector<App>> apps;

void EnumerateApps()
{
    if(apps.valid() == false)
    {
        apps.initialize();
    }

    apps->clear();

    DIR *directory = NULL;
    dirent *directoryEntry = NULL;

    directory = opendir("/apps");

    if(directory == NULL)
    {
        return;
    }

    while((directoryEntry = readdir(directory)) != NULL)
    {
        if(strcmp(directoryEntry->d_name, ".") == 0 || strcmp(directoryEntry->d_name, "..") == 0)
        {
            continue;
        }

        if(directoryEntry->d_type == DT_DIR)
        {
            auto app = App();

            app.name = directoryEntry->d_name;
            app.icon = string("/apps/") + app.name + "/icon.png";
            app.exe = string("/apps/") + app.name + "/" + app.name;

            apps->push_back(app);
        }
    }

    closedir(directory);
}

