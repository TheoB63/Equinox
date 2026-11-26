#pragma once

#include <string>
#include <filesystem>

namespace Equinox
{
    struct EditorSettings
    {
        // Style
        std::string activeStyle = "Custom";

        // Layout
        std::string activeLayout = "Default";

        // IBL / Environment
        float iblIntensity    = 1.0f;
        float skyboxIntensity = 1.0f;

        // Editor panels
        float cameraFlySpeed  = 50.0f;
        float thumbnailSize   = 64.0f;

        static EditorSettings Load(const std::filesystem::path& path);
        static void Save(const EditorSettings& settings, const std::filesystem::path& path);
    };
}