#include <Equinox.h>
#include <equinox/core/EntryPoint.h>

#include "VulkanApp.h"

namespace Equinox
{
    App* CreateApp(int argc, char** argv)
    {
        return new VulkanApp(argc, argv);
    }
}
