#include "net/net.hpp"
#include "net/arp.hpp"
#include "devicemanager/NetworkDevice.hpp"
#include "net/dhcp.hpp"
#include "net/dns.hpp"

box<vector<NetworkDevice *>> networkDevices;

void InitializeNetworking()
{
    networkDevices.initialize();

    auto devices = globalDeviceManager.GetDevices(DEVICE_TYPE_NETWORK);

    for(auto &device : devices)
    {
        NetworkDevice *networkDevice = (NetworkDevice *)device;

        if(networkDevice->NetworkType() != ARP_HTYPE_ETHERNET)
        {
            continue;            
        }

        DHCPNegotiate(networkDevice, [](NetworkDevice *networkDevice, bool success) {

            if(success == false)
            {
                DEBUG_OUT("[dhcp] Failed to negotiate DHCP for device %s", networkDevice->name());

                return;
            }

            ARPRequest(networkDevice, networkDevice->GatewayIP());
            ARPWaitReply(networkDevice->GatewayMac());

            DEBUG_OUT("[dhcp] MAC Address for %d.%d.%d.%d (gateway) is %02x:%02x:%02x:%02x:%02x:%02x",
                networkDevice->GatewayIP()[0], networkDevice->GatewayIP()[1], networkDevice->GatewayIP()[2], networkDevice->GatewayIP()[3],
                networkDevice->GatewayMac()[0], networkDevice->GatewayMac()[1], networkDevice->GatewayMac()[2],
                networkDevice->GatewayMac()[3], networkDevice->GatewayMac()[4], networkDevice->GatewayMac()[5]);

            ARPRequest(networkDevice, networkDevice->DNSIP());
            ARPWaitReply(networkDevice->DNSMac());

            DEBUG_OUT("[dhcp] MAC address for %d.%d.%d.%d (DNS) is %02x:%02x:%02x:%02x:%02x:%02x",
                networkDevice->DNSIP()[0], networkDevice->DNSIP()[1], networkDevice->DNSIP()[2], networkDevice->DNSIP()[3],
                networkDevice->DNSMac()[0], networkDevice->DNSMac()[1], networkDevice->DNSMac()[2],
                networkDevice->DNSMac()[3], networkDevice->DNSMac()[4], networkDevice->DNSMac()[5]);
            
            networkDevices->push_back(networkDevice);
        });
    }
}
