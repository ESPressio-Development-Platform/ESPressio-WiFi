#pragma once

#include <ESPressio_Persistence_Serializable_Security.hpp>
#include "ESPressio_WiFiConfiguration.hpp"

namespace ESPressio::WiFi {

class ProtectedWiFiConfigurationStore {
public:
    static Persistence::ProtectedSerializablePersistenceResult Save(
        Persistence::IFileStorage& storage,
        const char* path,
        const WiFiConfiguration& configuration,
        const Serializable::SerializationProtectionConfig& protection,
        const Persistence::SerializablePersistenceOptions& options = {}
    ) {
        return Persistence::SaveSerializable(storage, path, configuration, protection, options);
    }

    static Persistence::ProtectedSerializablePersistenceResult Load(
        Persistence::IFileStorage& storage,
        const char* path,
        WiFiConfiguration& configuration,
        const Serializable::SerializationProtectionConfig& protection,
        const Persistence::SerializablePersistenceOptions& options = {}
    ) {
        return Persistence::LoadSerializable(storage, path, configuration, protection, options);
    }
};

} // namespace ESPressio::WiFi
