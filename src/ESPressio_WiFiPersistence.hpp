#pragma once

#include <string>
#include <utility>
#include <ESPressio_Persistence_Serializable.hpp>
#include "ESPressio_IWiFiConfigurationStore.hpp"

namespace ESPressio::WiFi {

class WiFiConfigurationStore {
public:
    template<typename TConfiguration>
    static Persistence::SerializablePersistenceResult Save(
        Persistence::IFileStorage& storage, const char* path,
        const TConfiguration& configuration,
        const Persistence::SerializablePersistenceOptions& options = {}) {
        return Persistence::SaveSerializable(storage, path, configuration, options);
    }

    template<typename TConfiguration>
    static Persistence::SerializablePersistenceResult Load(
        Persistence::IFileStorage& storage, const char* path,
        TConfiguration& configuration,
        const Persistence::SerializablePersistenceOptions& options = {}) {
        return Persistence::LoadSerializable(storage, path, configuration, options);
    }

    template<typename TConfiguration>
    static Persistence::SerializablePersistenceResult Save(
        Persistence::IKeyValueStorage& storage, const char* key,
        const TConfiguration& configuration,
        const Persistence::SerializablePersistenceOptions& options = {}) {
        return Persistence::SaveSerializable(storage, key, configuration, options);
    }

    template<typename TConfiguration>
    static Persistence::SerializablePersistenceResult Load(
        Persistence::IKeyValueStorage& storage, const char* key,
        TConfiguration& configuration,
        const Persistence::SerializablePersistenceOptions& options = {}) {
        return Persistence::LoadSerializable(storage, key, configuration, options);
    }

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

class FileWiFiConfigurationStore final : public IWiFiConfigurationStore {
public:
    FileWiFiConfigurationStore(
        Persistence::IFileStorage& storage,
        std::string path,
        Persistence::SerializablePersistenceOptions options = {}
    ) : _storage(storage), _path(std::move(path)), _options(std::move(options)) {}

    WiFiConfigurationStoreResult Save(const WiFiConfiguration& configuration) override {
        return WiFiConfigurationStore::Translate(
            WiFiConfigurationStore::Save(_storage, _path.c_str(), configuration, _options)
        );
    }

    WiFiConfigurationStoreResult Load(WiFiConfiguration& configuration) override {
        return WiFiConfigurationStore::Translate(
            WiFiConfigurationStore::Load(_storage, _path.c_str(), configuration, _options)
        );
    }

private:
    Persistence::IFileStorage& _storage;
    std::string _path;
    Persistence::SerializablePersistenceOptions _options;
};

class KeyValueWiFiConfigurationStore final : public IWiFiConfigurationStore {
public:
    KeyValueWiFiConfigurationStore(
        Persistence::IKeyValueStorage& storage,
        std::string key,
        Persistence::SerializablePersistenceOptions options = {}
    ) : _storage(storage), _key(std::move(key)), _options(std::move(options)) {}

    WiFiConfigurationStoreResult Save(const WiFiConfiguration& configuration) override {
        return WiFiConfigurationStore::Translate(
            WiFiConfigurationStore::Save(_storage, _key.c_str(), configuration, _options)
        );
    }

    WiFiConfigurationStoreResult Load(WiFiConfiguration& configuration) override {
        return WiFiConfigurationStore::Translate(
            WiFiConfigurationStore::Load(_storage, _key.c_str(), configuration, _options)
        );
    }

private:
    Persistence::IKeyValueStorage& _storage;
    std::string _key;
    Persistence::SerializablePersistenceOptions _options;
};

} // namespace ESPressio::WiFi
