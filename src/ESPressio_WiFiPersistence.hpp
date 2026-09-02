#pragma once

#include <string_view>
#include <utility>
#include <ESPressio_Persistence_Serializable.hpp>
#include "ESPressio_IWiFiConfigurationStore.hpp"

namespace ESPressio::WiFi {

/// <summary>Static adapters for persisting arbitrary Serializable Wi-Fi configuration wrappers to file or key/value storage.</summary>
class WiFiConfigurationStore {
public:
    /// <summary>Saves a Serializable configuration to a file-oriented Persistence backend.</summary>
    template<typename TConfiguration>
    static Persistence::SerializablePersistenceResult Save(
        Persistence::IFileStorage& storage, const char* path,
        const TConfiguration& configuration,
        const Persistence::SerializablePersistenceOptions& options = {}) {
        return Persistence::SaveSerializable(storage, path, configuration, options);
    }

    /// <summary>Loads a Serializable configuration from a file-oriented Persistence backend.</summary>
    template<typename TConfiguration>
    static Persistence::SerializablePersistenceResult Load(
        Persistence::IFileStorage& storage, const char* path,
        TConfiguration& configuration,
        const Persistence::SerializablePersistenceOptions& options = {}) {
        return Persistence::LoadSerializable(storage, path, configuration, options);
    }

    /// <summary>Saves a Serializable configuration to a key/value Persistence backend.</summary>
    template<typename TConfiguration>
    static Persistence::SerializablePersistenceResult Save(
        Persistence::IKeyValueStorage& storage, const char* key,
        const TConfiguration& configuration,
        const Persistence::SerializablePersistenceOptions& options = {}) {
        return Persistence::SaveSerializable(storage, key, configuration, options);
    }

    /// <summary>Loads a Serializable configuration from a key/value Persistence backend.</summary>
    template<typename TConfiguration>
    static Persistence::SerializablePersistenceResult Load(
        Persistence::IKeyValueStorage& storage, const char* key,
        TConfiguration& configuration,
        const Persistence::SerializablePersistenceOptions& options = {}) {
        return Persistence::LoadSerializable(storage, key, configuration, options);
    }

    /// <summary>Translates a generic Serializable Persistence result into the Wi-Fi configuration-store result model.</summary>
    static WiFiConfigurationStoreResult Translate(
        const Persistence::SerializablePersistenceResult& result
    ) {
        if (result.Success()) return WiFiConfigurationStoreResult::Ok();
        if (result.Storage == Persistence::StorageStatus::NotFound) {
            return WiFiConfigurationStoreResult::Fail(
                WiFiConfigurationStoreStatus::NotFound,
                "Persisted WiFi configuration was not found"
            );
        }
        if (result.Status == Persistence::SerializablePersistenceStatus::StorageError) {
            return WiFiConfigurationStoreResult::Fail(
                WiFiConfigurationStoreStatus::StorageError,
                Persistence::SerializablePersistenceStatusName(result.Status)
            );
        }
        return WiFiConfigurationStoreResult::Fail(
            WiFiConfigurationStoreStatus::SerializationError,
            Persistence::SerializablePersistenceStatusName(result.Status)
        );
    }
};

/// <summary>IWiFiConfigurationStore implementation backed by an ESPressio file-storage backend.</summary>
class FileWiFiConfigurationStore final : public IWiFiConfigurationStore {
public:
    /// <summary>Constructs a file-backed configuration store and retains its path in externally preferred storage.</summary>
    FileWiFiConfigurationStore(
        Persistence::IFileStorage& storage,
        std::string_view path,
        Persistence::SerializablePersistenceOptions options = {}
    ) : _storage(storage), _options(std::move(options)) {
        _path.assign(path.data(), path.size());
    }

    /// <inheritdoc/>
    WiFiConfigurationStoreResult Save(const WiFiConfiguration& configuration) override {
        return WiFiConfigurationStore::Translate(
            WiFiConfigurationStore::Save(_storage, _path.c_str(), configuration, _options)
        );
    }

    /// <inheritdoc/>
    WiFiConfigurationStoreResult Load(WiFiConfiguration& configuration) override {
        return WiFiConfigurationStore::Translate(
            WiFiConfigurationStore::Load(_storage, _path.c_str(), configuration, _options)
        );
    }

private:
    Persistence::IFileStorage& _storage;
    WiFiString _path;
    Persistence::SerializablePersistenceOptions _options;
};

/// <summary>IWiFiConfigurationStore implementation backed by an ESPressio key/value storage backend.</summary>
class KeyValueWiFiConfigurationStore final : public IWiFiConfigurationStore {
public:
    /// <summary>Constructs a key/value-backed configuration store and retains its key in externally preferred storage.</summary>
    KeyValueWiFiConfigurationStore(
        Persistence::IKeyValueStorage& storage,
        std::string_view key,
        Persistence::SerializablePersistenceOptions options = {}
    ) : _storage(storage), _options(std::move(options)) {
        _key.assign(key.data(), key.size());
    }

    /// <inheritdoc/>
    WiFiConfigurationStoreResult Save(const WiFiConfiguration& configuration) override {
        return WiFiConfigurationStore::Translate(
            WiFiConfigurationStore::Save(_storage, _key.c_str(), configuration, _options)
        );
    }

    /// <inheritdoc/>
    WiFiConfigurationStoreResult Load(WiFiConfiguration& configuration) override {
        return WiFiConfigurationStore::Translate(
            WiFiConfigurationStore::Load(_storage, _key.c_str(), configuration, _options)
        );
    }

private:
    Persistence::IKeyValueStorage& _storage;
    WiFiString _key;
    Persistence::SerializablePersistenceOptions _options;
};

} // namespace ESPressio::WiFi
