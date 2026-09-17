#include "eqnpch.h"
#include "equinox/resources/libraries/ShaderLibrary.h"
#include "equinox/resources/ResourceDB.h"

namespace Equinox
{
	std::shared_mutex ShaderLibrary::s_Mutex;
	std::unordered_map<UUID, ShaderLibrary::ShaderRecord, UUIDHash> ShaderLibrary::s_Shaders;
	std::unordered_map<std::string, UUID> ShaderLibrary::s_NameToUuidMap;

	void ShaderLibrary::Init()
	{
		std::unique_lock lock(s_Mutex);
		EQN_CORE_INFO("Initialized Shader Library");
	}

	void ShaderLibrary::Shutdown()
	{
		std::unique_lock lock(s_Mutex);
		s_Shaders.clear();
		EQN_CORE_INFO("Cleared Shader Library");
	}

	bool ShaderLibrary::Add(std::shared_ptr<Shader> shader)
	{
		if (!shader)
		{
			EQN_CORE_ERROR("Attempted to add null Shader");
			return false;
		}

		UUID uuid = shader->GetUUID();
		std::string name = shader->GetName();
		std::unique_lock lock(s_Mutex);

		auto [it, inserted] = s_Shaders.try_emplace(uuid,
			ShaderRecord{ shader, name,"", fs::file_time_type() }
		);

		if (!inserted)
		{
			EQN_CORE_WARN("Shader with UUID {0} already exists! Overwriting...", uuid.ToString());
			it->second = { shader,name, "", fs::file_time_type() };
		}

		// Update name mapping
		if (!name.empty())
		{
			s_NameToUuidMap[name] = uuid;
		}

		return true;
	}

	bool ShaderLibrary::Remove(const UUID& uuid)
	{
		std::unique_lock lock(s_Mutex);
		return s_Shaders.erase(uuid) > 0;
	}

	bool ShaderLibrary::Contains(const UUID& uuid)
	{
		std::shared_lock lock(s_Mutex);
		return s_Shaders.find(uuid) != s_Shaders.end();
	}

	std::shared_ptr<Shader> ShaderLibrary::Get(const UUID& uuid)
	{
		std::shared_lock lock(s_Mutex);
		auto it = s_Shaders.find(uuid);
		return it != s_Shaders.end() ? it->second.Shader : nullptr;
	}

	std::shared_ptr<Shader> ShaderLibrary::Get(const std::string& name)
	{
		std::shared_lock lock(s_Mutex);
		auto uuidIt = s_NameToUuidMap.find(name);
		if (uuidIt == s_NameToUuidMap.end())
		{
			return nullptr;
		}

		auto shaderIt = s_Shaders.find(uuidIt->second);
		return shaderIt != s_Shaders.end() ? shaderIt->second.Shader : nullptr;
	}

	std::vector<UUID> ShaderLibrary::GetAllUuids()
	{
		std::shared_lock lock(s_Mutex);
		std::vector<UUID> uuids;
		uuids.reserve(s_Shaders.size());
		for (const auto& [uuid, _] : s_Shaders)
			uuids.push_back(uuid);
		return uuids;
	}

	std::unordered_map<UUID, ShaderLibrary::ShaderRecord, UUIDHash> ShaderLibrary::GetAllShaders()
	{
		return s_Shaders;
	}

	std::shared_ptr<Shader> ShaderLibrary::Load(const fs::path& filePath)
	{
		if (!fs::exists(filePath))
		{
			EQN_CORE_ERROR("Shader file not found: {0}", filePath.string());
			return nullptr;
		}

		UUID uuid = ResourceDB::PathToUuid(filePath);
		if (auto existing = Get(uuid))
		{
			EQN_CORE_INFO("Shader already loaded: {0}", uuid.ToString());
			return existing;
		}

		auto shader = Shader::Create(filePath);
		if (!shader)
		{
			EQN_CORE_ERROR("Failed to compile shader from {0}", filePath.string());
			return nullptr;
		}

		auto modTime = fs::last_write_time(filePath);
		std::string name = filePath.filename().stem().string();

		std::unique_lock lock(s_Mutex);
		s_Shaders[uuid] = { shader,name, filePath, modTime };
		shader->SetUUID(uuid);
		shader->SetName(name);

		if (!name.empty())
		{
			s_NameToUuidMap[name] = uuid;
		}

		EQN_CORE_INFO("Loaded Shader {0} from {1}", uuid.ToString(), filePath.string());
		return shader;
	}

	std::shared_ptr<Shader> ShaderLibrary::LoadOrGet(const fs::path& filePath)
	{
		UUID uuid = ResourceDB::PathToUuid(filePath);
		if (auto shader = Get(uuid))
		{
			return shader;
		}
		return Load(filePath);
	}

	bool ShaderLibrary::Reload(const UUID& uuid)
	{
		std::unique_lock lock(s_Mutex);
		auto it = s_Shaders.find(uuid);
		if (it == s_Shaders.end())
		{
			EQN_CORE_WARN("Cannot reload non-existent Shader {0}", uuid.ToString());
			return false;
		}

		auto& record = it->second;
		if (record.SourcePath.empty())
		{
			EQN_CORE_INFO("Shader {0} not reloadable (no source path)", uuid.ToString());
			return false;
		}

		if (!fs::exists(record.SourcePath))
		{
			EQN_CORE_ERROR("Shader source missing: {0}", record.SourcePath.string());
			return false;
		}

		const auto newTime = fs::last_write_time(record.SourcePath);
		if (newTime <= record.LastModified)
			return true; // Already up-to-date

		try
		{
			auto newShader = Shader::Create(record.SourcePath);
			if (!newShader)
				throw std::runtime_error("Compilation failed");

			newShader->SetUUID(uuid);
			record.Shader = newShader;
			record.LastModified = newTime;

			EQN_CORE_INFO("Successfully reloaded Shader {0}", uuid.ToString());
			return true;
		}
		catch (const std::exception& e)
		{
			EQN_CORE_ERROR("Failed to reload Shader {0}: {1}", uuid.ToString(), e.what());
			return false;
		}
	}

	void ShaderLibrary::ReloadAll()
	{
		std::unique_lock lock(s_Mutex);
		EQN_CORE_INFO("Reloading all shaders...");

		size_t successCount = 0;
		size_t failCount = 0;

		for (auto& [uuid, record] : s_Shaders)
		{
			if (record.SourcePath.empty()) continue;

			if (!fs::exists(record.SourcePath))
			{
				EQN_CORE_ERROR("Shader source missing: {0}", record.SourcePath.string());
				failCount++;
				continue;
			}

			const auto newTime = fs::last_write_time(record.SourcePath);
			if (newTime <= record.LastModified)
				continue;

			try
			{
				auto newShader = Shader::Create(record.SourcePath);
				if (!newShader)
					throw std::runtime_error("Compilation failed");

				newShader->SetUUID(uuid);
				record.Shader = newShader;
				record.LastModified = newTime;
				successCount++;
			}
			catch (const std::exception& e)
			{
				EQN_CORE_ERROR("Reload failed for {0}: {1}", uuid.ToString(), e.what());
				failCount++;
			}
		}

		EQN_CORE_INFO("Reloaded Shaders: {0} succeeded, {1} failed", successCount, failCount);
	}
}