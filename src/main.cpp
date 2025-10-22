#include <iostream>
#include <filesystem>
#include <windows.h>
#include "fluid.h"

int main()
{
    wchar_t path[MAX_PATH];
    if (!GetModuleFileNameW(NULL, path, (DWORD)MAX_PATH) > 0) {
        return -1;
    }
    std::filesystem::path p(path);
    std::filesystem::current_path(p.parent_path());

    AddDllDirectory(p.parent_path().append("libs").c_str());
    AddDllDirectory(p.parent_path().append("libs\\SFML-2.6.0\\bin").c_str());

    Parameters params;
    Main main(params);
    main.run();
}