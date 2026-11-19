#pragma once

#include "equinox/resources/AssetImporter.h"
#include "equinox/core/EquinoxTypes.h"
#include <vector>

namespace Equinox
{
	struct ShaderAssetData : public AssetData
	{
		std::vector<u32> VertexSpirV;
		std::vector<u32> FragmentSpirV;
		std::string SourcePath; // Original source path for debug/reload
	};

	class ShaderImporter : public AssetImporter
	{
	public:
		bool Import(const std::filesystem::path& source, const std::filesystem::path& destination) override;
	};
}