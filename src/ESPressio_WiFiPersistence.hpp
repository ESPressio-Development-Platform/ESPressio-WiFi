#pragma once

#include <ESPressio_Persistence_Serializable.hpp>
#include "ESPressio_WiFiConfiguration.hpp"

namespace ESPressio::WiFi {

class WiFiConfigurationStore {
public:
    static Persistence::SerializablePersistenceResult Save(
        Persistence::IFileStorage& storage,
        const char* path,
        const WiFiConfiguration& configuration,
        const Persistence::SerializablePersistenceOptions& options = {}
    ) {
        return Persistence::SaveSerializable(storage, path, configuration, options);
    }

    static Persistence::SerializablePersistenceResult Load(
        Persistence::IFileStorage& storage,
        const char* path,
        WiFiConfiguration& configuration,
        const Persistence::SerializablePersistenceOptions& options = {}
    ) {
        return Persistence::LoadSerializable(storage, path, configuration, options);
    }

    static Persistence::SerializablePersistenceResult Save(
        Persistence::IKeyValueStorage& storage,
        const char* key,
        const WiFiConfiguration& configuration,
        const Persistence::SerializablePersistenceOptions& options = {}
    ) {
        return Persistence::SaveSerializable(storage, key, configuration, options);
    }

    static Persistence::SerializablePersistenceResult Load(
        Persistence::IKeyValueStorage& storage,
        const char* key,
        WiFiConfiguration& configuration,
        const Persistence::SerializablePersistenceOptions& options = {}
    ) {
        return Persistence::LoadSerializable(storage, key, configuration, options);
    }
};

} // namespace ESPressio::WiFi
