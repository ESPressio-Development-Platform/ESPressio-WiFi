#pragma once

#include <ESPressio_Persistence_Serializable_Security.hpp>

namespace ESPressio::WiFi {

class ProtectedWiFiConfigurationStore {
public:
    template<typename TConfiguration>
    static Persistence::ProtectedSerializablePersistenceResult Save(
        Persistence::IFileStorage& storage, const char* path,
        const TConfiguration& configuration,
        const Serializable::SerializationProtectionConfig& protection,
        const Persistence::SerializablePersistenceOptions& options = {}) {
        return Persistence::SaveSerializable(storage, path, configuration, protection, options);
    }

    template<typename TConfiguration>
    static Persistence::ProtectedSerializablePersistenceResult Load(
        Persistence::IFileStorage& storage, const char* path,
        TConfiguration& configuration,
        const Serializable::SerializationProtectionConfig& protection,
        const Persistence::SerializablePersistenceOptions& options = {}) {
        return Persistence::LoadSerializable(storage, path, configuration, protection, options);
    }

    template<typename TConfiguration>
    static Persistence::ProtectedSerializablePersistenceResult Save(
        Persistence::IKeyValueStorage& storage, const char* key,
        const TConfiguration& configuration,
        const Serializable::SerializationProtectionConfig& protection,
        const Persistence::SerializablePersistenceOptions& options = {}) {
        return Persistence::SaveSerializable(storage, key, configuration, protection, options);
    }

    template<typename TConfiguration>
    static Persistence::ProtectedSerializablePersistenceResult Load(
        Persistence::IKeyValueStorage& storage, const char* key,
        TConfiguration& configuration,
        const Serializable::SerializationProtectionConfig& protection,
        const Persistence::SerializablePersistenceOptions& options = {}) {
        return Persistence::LoadSerializable(storage, key, configuration, protection, options);
    }
};

} // namespace ESPressio::WiFi
