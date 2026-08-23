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
    source.Client.Selection.AutomaticSelection = true;
    source.Client.Selection.ScanOnStartup = true;

    ClientNetworkProfile studio;
    studio.SSID = "Studio";
    studio.Password = "studio-password";
    studio.Priority = 300;
    studio.Addressing = AddressMode::Static;
    studio.StaticNetwork.Address = IPv4Address(192,168,1,50);
    source.Client.Networks.push_back(studio);

    ClientNetworkProfile mobile;
    mobile.SSID = "Mobile";
    mobile.Password = "mobile-password";
    mobile.Priority = 100;
    source.Client.Networks.push_back(mobile);

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
    assert(restored.Client.Selection.AutomaticSelection);
    assert(restored.Client.Networks.size() == 2);
    assert(restored.Client.Networks[0].SSID == "Studio");
    assert(restored.Client.Networks[0].Password == "studio-password");
    assert(restored.Client.Networks[0].Priority == 300);
    assert(restored.Client.Networks[0].Addressing == AddressMode::Static);
    assert(restored.Client.Networks[0].StaticNetwork.Address == IPv4Address(192,168,1,50));
    assert(restored.Client.Networks[1].SSID == "Mobile");
    assert(restored.Reconnect.MaximumAttempts == 7);
    assert(restored.TxPowerDbm == 15);
    assert(restored.PowerSave);

    const auto apProperties = AccessPointConfiguration::GetSerializableProperties();
    const auto clientProperties = ClientConfiguration::GetSerializableProperties();
    const auto profileProperties = ClientNetworkProfile::GetSerializableProperties();
    static_assert(std::tuple_size<decltype(apProperties)>::value >= 3, "AP schema missing properties");
    static_assert(std::tuple_size<decltype(clientProperties)>::value >= 6, "Client schema missing remembered networks");
    static_assert(std::tuple_size<decltype(profileProperties)>::value >= 2, "Profile schema missing properties");
    assert(std::get<2>(apProperties).IsSensitive());
    assert(std::get<2>(clientProperties).IsSensitive());
    assert(std::get<1>(profileProperties).IsSensitive());
    return 0;
}
