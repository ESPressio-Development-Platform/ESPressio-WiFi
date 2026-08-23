#pragma once

#include <ESPressio_Persistence_Serializable.hpp>

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
};

} // namespace ESPressio::WiFi
