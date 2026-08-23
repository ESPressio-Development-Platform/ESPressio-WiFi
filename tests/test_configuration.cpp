#include <cassert>
#include <ESPressio_Serializable_Binary.hpp>
#include <ESPressio_WiFiConfiguration.hpp>

using namespace ESPressio;
using namespace ESPressio::WiFi;

int main() {
    WiFiConfiguration source;
    source.Mode = WiFiMode::AccessPointClient;
    source.Hostname = "camera-controller";
    source.AccessPoint.SSID = "Camera-Control";
    source.AccessPoint.Password = "ap-password";
    source.AccessPoint.Channel = 6;
    source.AccessPoint.DHCP.LeaseStart = IPv4Address(10,20,30,10);
    source.AccessPoint.DHCP.LeaseEnd = IPv4Address(10,20,30,50);
    source.AccessPoint.DHCP.LeaseDurationSeconds = 3600;
    source.Client.Enabled = true;
    source.Client.SSID = "Studio";
    source.Client.Password = "client-password";
    source.Client.Addressing = AddressMode::Static;
    source.Client.StaticNetwork.Address = IPv4Address(192,168,1,50);
    source.Reconnect.BackoffMultiplier = 1.5f;
    source.Reconnect.MaximumAttempts = 7;
    source.TxPowerDbm = 15;
    source.PowerSave = true;

    Serializable::BinaryArchive archive;
    source.Serialize(archive);
    const auto bytes = archive.GetData();
    assert(!bytes.empty());

    Serializable::BinaryArchive loadedArchive;
    assert(loadedArchive.Load(bytes.data(), bytes.size()));
    WiFiConfiguration restored;
    const auto result = restored.DeserializeDetailed(loadedArchive);
    assert(result.Success());

    assert(restored.Mode == WiFiMode::AccessPointClient);
    assert(restored.Hostname == "camera-controller");
    assert(restored.AccessPoint.SSID == "Camera-Control");
    assert(restored.AccessPoint.Password == "ap-password");
    assert(restored.AccessPoint.Channel == 6);
    assert(restored.AccessPoint.DHCP.LeaseStart == IPv4Address(10,20,30,10));
    assert(restored.AccessPoint.DHCP.LeaseEnd == IPv4Address(10,20,30,50));
    assert(restored.Client.Enabled);
    assert(restored.Client.SSID == "Studio");
    assert(restored.Client.Password == "client-password");
    assert(restored.Client.Addressing == AddressMode::Static);
    assert(restored.Client.StaticNetwork.Address == IPv4Address(192,168,1,50));
    assert(restored.Reconnect.MaximumAttempts == 7);
    assert(restored.TxPowerDbm == 15);
    assert(restored.PowerSave);

    const auto properties = WiFiConfiguration::GetSerializableProperties();
    (void)properties;
    const auto apProperties = AccessPointConfiguration::GetSerializableProperties();
    const auto clientProperties = ClientConfiguration::GetSerializableProperties();
    static_assert(std::tuple_size<decltype(apProperties)>::value >= 3, "AP schema missing properties");
    static_assert(std::tuple_size<decltype(clientProperties)>::value >= 3, "Client schema missing properties");
    assert(std::get<2>(apProperties).IsSensitive());
    assert(std::get<2>(clientProperties).IsSensitive());
    return 0;
}
