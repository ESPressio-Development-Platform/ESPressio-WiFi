#pragma once

#include <string>
#include <utility>
#include <ESPressio_Persistence_Serializable_Security.hpp>
#include "ESPressio_IWiFiConfigurationStore.hpp"

namespace ESPressio::WiFi {

class ProtectedWiFiConfigurationStore {
public:
    template<typename TConfiguration>
    static Persistence::ProtectedSerializablePersistenceResult Save(
        Persistence::IFileStorage& storage,
        const char* path,
        const TConfiguration& configuration,
        const Serializable::SerializationProtectionConfig& protection,
        bool preferAtomicFileReplace = true
    ) {
        return Persistence::SaveSerializable(storage, path, configuration, protection, preferAtomicFileReplace);
    }

    template<typename TConfiguration>
    static Persistence::ProtectedSerializablePersistenceResult Load(
        Persistence::IFileStorage& storage,
        const char* path,
        TConfiguration& configuration,
        const Serializable::SerializationProtectionConfig& protection
    ) {
        return Persistence::LoadSerializable(storage, path, configuration, protection);
    }

    template<typename TConfiguration>
    static Persistence::ProtectedSerializablePersistenceResult Save(
        Persistence::IKeyValueStorage& storage,
        const char* key,
        const TConfiguration& configuration,
        const Serializable::SerializationProtectionConfig& protection
    ) {
        return Persistence::SaveSerializable(storage, key, configuration, protection);
    }

    template<typename TConfiguration>
    static Persistence::ProtectedSerializablePersistenceResult Load(
        Persistence::IKeyValueStorage& storage,
        const char* key,
        TConfiguration& configuration,
        const Serializable::SerializationProtectionConfig& protection
    ) {
        return Persistence::LoadSerializable(storage, key, configuration, protection);
    }

    static WiFiConfigurationStoreResult Translate(
        const Persistence::ProtectedSerializablePersistenceResult& result
    ) {
        if (result.Success()) return WiFiConfigurationStoreResult::Ok();
        if (result.Storage == Persistence::StorageStatus::NotFound) {
            return WiFiConfigurationStoreResult::Fail(
                WiFiConfigurationStoreStatus::NotFound,
                "Protected WiFi configuration was not found"
            );
        }
        if (result.Storage != Persistence::StorageStatus::Success) {
            return WiFiConfigurationStoreResult::Fail(
                WiFiConfigurationStoreStatus::StorageError,
                "Persistence backend rejected protected WiFi configuration"
            );
        }

        std::string message = Serializable::ProtectedSerializationStatusName(
            result.Serialization.Status
        );
        if (!result.Serialization.SecurityResult.Message.empty()) {
            message += ": ";
            message += result.Serialization.SecurityResult.Message;
        }
        return WiFiConfigurationStoreResult::Fail(
            WiFiConfigurationStoreStatus::ProtectionError,
            std::move(message)
        );
    }
};

class ProtectedFileWiFiConfigurationStore final : public IWiFiConfigurationStore {
public:
    ProtectedFileWiFiConfigurationStore(
        Persistence::IFileStorage& storage,
        std::string path,
        Serializable::SerializationProtectionConfig protection,
        bool preferAtomicFileReplace = true
    ) : _storage(storage), _path(std::move(path)), _protection(std::move(protection)),
        _preferAtomicFileReplace(preferAtomicFileReplace) {}

    WiFiConfigurationStoreResult Save(const WiFiConfiguration& configuration) override {
        return ProtectedWiFiConfigurationStore::Translate(
            ProtectedWiFiConfigurationStore::Save(
                _storage, _path.c_str(), configuration, _protection, _preferAtomicFileReplace
            )
        );
    }

    WiFiConfigurationStoreResult Load(WiFiConfiguration& configuration) override {
        return ProtectedWiFiConfigurationStore::Translate(
            ProtectedWiFiConfigurationStore::Load(_storage, _path.c_str(), configuration, _protection)
        );
    }

private:
    Persistence::IFileStorage& _storage;
    std::string _path;
    Serializable::SerializationProtectionConfig _protection;
    bool _preferAtomicFileReplace = true;
};

class ProtectedKeyValueWiFiConfigurationStore final : public IWiFiConfigurationStore {
public:
    ProtectedKeyValueWiFiConfigurationStore(
        Persistence::IKeyValueStorage& storage,
        std::string key,
        Serializable::SerializationProtectionConfig protection
    ) : _storage(storage), _key(std::move(key)), _protection(std::move(protection)) {}

    WiFiConfigurationStoreResult Save(const WiFiConfiguration& configuration) override {
        return ProtectedWiFiConfigurationStore::Translate(
            ProtectedWiFiConfigurationStore::Save(_storage, _key.c_str(), configuration, _protection)
        );
    }

    WiFiConfigurationStoreResult Load(WiFiConfiguration& configuration) override {
        return ProtectedWiFiConfigurationStore::Translate(
            ProtectedWiFiConfigurationStore::Load(_storage, _key.c_str(), configuration, _protection)
        );
    }

private:
    Persistence::IKeyValueStorage& _storage;
    std::string _key;
    Serializable::SerializationProtectionConfig _protection;
};

} // namespace ESPressio::WiFi
