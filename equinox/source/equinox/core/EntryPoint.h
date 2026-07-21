#pragma once

#include "equinox/core/App.h"
#include "equinox/core/Log.h"

#include <iostream>

int main(int argc, char** argv) 
{
    Equinox::Log::Init();
    Equinox::App* app = Equinox::CreateApp(argc,argv);

    if (!app)
    {
        EQN_CORE_CRITICAL("Failed to create app. Exiting.");

        #ifdef _DEBUG
            std::cerr << "Press Enter to exit...";
            std::cin.ignore();
        #endif
    }

    app->Run();
    delete app;
}