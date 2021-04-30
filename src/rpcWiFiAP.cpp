/*
 ESP8266WiFiSTA.cpp - rpcWiFi library for esp8266

 Copyright (c) 2014 Ivan Grokhotkov. All rights reserved.
 This file is part of the esp8266 core for Arduino environment.

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

 Reworked on 28 Dec 2015 by Markus Sattler

 */

#include "rpcWiFi.h"
#include "rpcWiFiGeneric.h"
#include "rpcWiFiAP.h"

extern "C"
{
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
}
#define MAX_STA_CONNECT_NUM 10

// -----------------------------------------------------------------------------------------------------------------------
// ----------------------------------------------------- AP function -----------------------------------------------------
// -----------------------------------------------------------------------------------------------------------------------

/**
 * Set up an access point
 * @param ssid              Pointer to the SSID (max 63 char).
 * @param passphrase        (for WPA2 min 8 char, for open use NULL)
 * @param channel           rpcWiFi channel number, 1 - 13.
 * @param ssid_hidden       Network cloaking (0 = broadcast SSID, 1 = hide SSID)
 * @param max_connection    Max simultaneous connected clients, 1 - 4.
*/
bool rpcWiFiAPClass::softAP(const char *ssid, const char *passphrase, int channel, int ssid_hidden, int max_connection)
{
    //passphrase = NULL;
    if (!rpcWiFi.enableAP(true))
    {
        // enable AP failed
        log_e("enable AP first!");
        return false;
    }

    if (!ssid || *ssid == 0)
    {
        // fail SSID missing
        log_e("SSID missing!");
        return false;
    }

    if ((passphrase != NULL) && strlen(passphrase) > 0 && strlen(passphrase) < 8)
    {
        // fail passphrase too short
        log_e("passphrase too short!");
        return false;
    }

    rtw_security_t authmode = RTW_SECURITY_WPA2_AES_PSK;
    if (!passphrase || strlen(passphrase) == 0)
    {
        authmode = RTW_SECURITY_OPEN;
    }
    else
    {
        authmode = RTW_SECURITY_WPA2_AES_PSK;
    }

    if (ssid_hidden)
    {
        if (RTW_SUCCESS != wifi_start_ap_with_hidden_ssid((char *)ssid, authmode, (char *)passphrase, strlen(ssid), strlen(passphrase), channel))
        {
            return false;
        }
    }
    else
    {
        if (RTW_SUCCESS != wifi_start_ap((char *)ssid, authmode, (char *)passphrase, strlen(ssid), strlen(passphrase), channel))
        {
            return false;
        }
    }
    rpc_tcpip_adapter_ip_info_t info;
    info.ip.addr = static_cast<uint32_t>(_local_ip);
    info.gw.addr = static_cast<uint32_t>(_gateway);
    info.netmask.addr = static_cast<uint32_t>(_subnet);
    new_tcpip_adapter_dhcps_stop(RPC_TCPIP_ADAPTER_IF_AP);
    if (new_tcpip_adapter_set_ip_info(RPC_TCPIP_ADAPTER_IF_AP, &info) == ESP_OK)
    {
        new_dhcps_lease_t lease;
        lease.enable = true;
        lease.start_ip.addr = static_cast<uint32_t>(_local_ip) + (1 << 24);
        lease.end_ip.addr = static_cast<uint32_t>(_local_ip) + (11 << 24);

        new_tcpip_adapter_dhcps_option(
            (rpc_tcpip_adapter_option_mode_t)RPC_TCPIP_ADAPTER_OP_SET,
            (rpc_tcpip_adapter_option_id_t)RPC_TCPIP_ADAPTER_REQUESTED_IP_ADDRESS,
            (void *)&lease, sizeof(new_dhcps_lease_t));

        return new_tcpip_adapter_dhcps_start(RPC_TCPIP_ADAPTER_IF_AP) == ESP_OK;
    }
    return true;
}


/**
 * Set up an access point
 * @param ssid              Pointer to the SSID (max 63 char).
 * @param passphrase        (for WPA2 min 8 char, for open use NULL)
 * @param channel           rpcWiFi channel number, 1 - 13.
 * @param ssid_hidden       Network cloaking (0 = broadcast SSID, 1 = hide SSID)
 * @param max_connection    Max simultaneous connected clients, 1 - 4.
*/
bool rpcWiFiAPClass::softAP(const char *ssid, int channel, int ssid_hidden, int max_connection)
{
    //passphrase = NULL;
    if (!rpcWiFi.enableAP(true))
    {
        // enable AP failed
        log_e("enable AP first!");
        return false;
    }

    if (!ssid || *ssid == 0)
    {
        // fail SSID missing
        log_e("SSID missing!");
        return false;
    }

    rtw_security_t authmode = RTW_SECURITY_OPEN;
   
    if (ssid_hidden)
    {
        if (RTW_SUCCESS != wifi_start_ap_with_hidden_ssid((char *)ssid, authmode, NULL, strlen(ssid), 0, channel))
        {
            return false;
        }
    }
    else
    {
        if (RTW_SUCCESS != wifi_start_ap((char *)ssid, authmode, NULL, strlen(ssid), 0, channel))
        {
            return false;
        }
    }
    rpc_tcpip_adapter_ip_info_t info;
    info.ip.addr = static_cast<uint32_t>(_local_ip);
    info.gw.addr = static_cast<uint32_t>(_gateway);
    info.netmask.addr = static_cast<uint32_t>(_subnet);
    new_tcpip_adapter_dhcps_stop(RPC_TCPIP_ADAPTER_IF_AP);
    if (new_tcpip_adapter_set_ip_info(RPC_TCPIP_ADAPTER_IF_AP, &info) == ESP_OK)
    {
        new_dhcps_lease_t lease;
        lease.enable = true;
        lease.start_ip.addr = static_cast<uint32_t>(_local_ip) + (1 << 24);
        lease.end_ip.addr = static_cast<uint32_t>(_local_ip) + (11 << 24);

        new_tcpip_adapter_dhcps_option(
            (rpc_tcpip_adapter_option_mode_t)RPC_TCPIP_ADAPTER_OP_SET,
            (rpc_tcpip_adapter_option_id_t)RPC_TCPIP_ADAPTER_REQUESTED_IP_ADDRESS,
            (void *)&lease, sizeof(new_dhcps_lease_t));

        return new_tcpip_adapter_dhcps_start(RPC_TCPIP_ADAPTER_IF_AP) == ESP_OK;
    }
    return true;
}



/**
 * Configure access point
 * @param local_ip      access point IP
 * @param gateway       gateway IP
 * @param subnet        subnet mask
 */
bool rpcWiFiAPClass::softAPConfig(IPAddress local_ip, IPAddress gateway, IPAddress subnet)
{

    if (!rpcWiFi.enableAP(true))
    {
        // enable AP failed
        return false;
    }

    //esp_wifi_start();
    _local_ip = local_ip;
    _gateway = gateway;
    _subnet = subnet;

    return false;
}

/**
 * Disconnect from the network (close AP)
 * @param wifioff disable mode?
 * @return one value of wl_status_t enum
 */
bool rpcWiFiAPClass::softAPdisconnect(bool wifioff)
{
    bool ret;
    wifi_config_t conf;

    if (rpcWiFiGenericClass::getMode() == WIFI_MODE_NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }

    ret = wifi_off() == RTW_SUCCESS;

    if (wifioff)
    {
        ret = rpcWiFi.enableAP(false) == RTW_SUCCESS;
    }
    else
    {
        ret = rpcWiFi.enableAP(true) == RTW_SUCCESS;
    }

    return ret;
}

/**
 * Get the count of the Station / client that are connected to the softAP interface
 * @return Stations count
 */
uint8_t rpcWiFiAPClass::softAPgetStationNum()
{
    wifi_sta_list_t clients;
    if (rpcWiFiGenericClass::getMode() == WIFI_MODE_NULL)
    {
        return 0;
    }

    struct
    {
        int count;
        rtw_mac_t mac_list[MAX_STA_CONNECT_NUM];
    } client_info;

    client_info.count = MAX_STA_CONNECT_NUM;

    if (wifi_get_associated_client_list(&client_info, sizeof(client_info)) == RTW_SUCCESS)
    {
        return client_info.count;
    }
    return 0;
}

/**
 * Get the softAP interface IP address.
 * @return IPAddress softAP IP
 */
IPAddress rpcWiFiAPClass::softAPIP()
{
    rpc_tcpip_adapter_ip_info_t ip;
    if (rpcWiFiGenericClass::getMode() == WIFI_MODE_NULL)
    {
        return IPAddress();
    }
    new_tcpip_adapter_get_ip_info(RPC_TCPIP_ADAPTER_IF_AP, &ip);

    return IPAddress(ip.ip.addr);
}

/**
 * Get the softAP broadcast IP address.
 * @return IPAddress softAP broadcastIP
 */
IPAddress rpcWiFiAPClass::softAPBroadcastIP()
{
    rpc_tcpip_adapter_ip_info_t ip;
    if (rpcWiFiGenericClass::getMode() == WIFI_MODE_NULL)
    {
        return IPAddress();
    }
    new_tcpip_adapter_get_ip_info(RPC_TCPIP_ADAPTER_IF_AP, &ip);
    return rpcWiFiGenericClass::calculateBroadcast(IPAddress(ip.gw.addr), IPAddress(ip.netmask.addr));
}

/**
 * Get the softAP network ID.
 * @return IPAddress softAP networkID
 */
IPAddress rpcWiFiAPClass::softAPNetworkID()
{
    rpc_tcpip_adapter_ip_info_t ip;
    if (rpcWiFiGenericClass::getMode() == WIFI_MODE_NULL)
    {
        return IPAddress();
    }
    new_tcpip_adapter_get_ip_info(RPC_TCPIP_ADAPTER_IF_AP, &ip);
    return rpcWiFiGenericClass::calculateNetworkID(IPAddress(ip.gw.addr), IPAddress(ip.netmask.addr));
}

/**
 * Get the softAP subnet CIDR.
 * @return uint8_t softAP subnetCIDR
 */
uint8_t rpcWiFiAPClass::softAPSubnetCIDR()
{
    rpc_tcpip_adapter_ip_info_t ip;
    if (rpcWiFiGenericClass::getMode() == WIFI_MODE_NULL)
    {
        return (uint8_t)0;
    }
    new_tcpip_adapter_get_ip_info(RPC_TCPIP_ADAPTER_IF_AP, &ip);
    return rpcWiFiGenericClass::calculateSubnetCIDR(IPAddress(ip.netmask.addr));
}

/**
 * Get the softAP interface MAC address.
 * @param mac   pointer to uint8_t array with length WL_MAC_ADDR_LENGTH
 * @return      pointer to uint8_t*
 */
uint8_t *rpcWiFiAPClass::softAPmacAddress(uint8_t *mac)
{
    if (rpcWiFiGenericClass::getMode() != WIFI_MODE_NULL)
    {
        String macStr = softAPmacAddress();
        for (int i = 0; i < 6; i++)
        {
            mac[i] = (uint8_t)htoi(macStr.substring(i * 3, i * 3 + 2).c_str());
        }
    }
    return mac;
}

/**
 * Get the softAP interface MAC address.
 * @return String mac
 */
String rpcWiFiAPClass::softAPmacAddress(void)
{
    char macStr[18] = {0};
    if (rpcWiFiGenericClass::getMode() == WIFI_MODE_NULL)
    {
        return String();
    }
    new_tcpip_adapter_get_mac(RPC_TCPIP_ADAPTER_IF_AP, (uint8_t *)macStr);

    return String(macStr);
}

/**
 * Get the softAP interface Host name.
 * @return char array hostname
 */
const char *rpcWiFiAPClass::softAPgetHostname()
{
    const char *hostname = NULL;
    if (rpcWiFiGenericClass::getMode() == WIFI_MODE_NULL)
    {
        return hostname;
    }
    if (new_tcpip_adapter_get_hostname(RPC_TCPIP_ADAPTER_IF_AP, &hostname))
    {
        return hostname;
    }
}

/**
 * Set the softAP    interface Host name.
 * @param  hostname  pointer to const string
 * @return true on   success
 */
bool rpcWiFiAPClass::softAPsetHostname(const char *hostname)
{
    if (rpcWiFiGenericClass::getMode() == WIFI_MODE_NULL)
    {
        return false;
    }
    return new_tcpip_adapter_set_hostname(RPC_TCPIP_ADAPTER_IF_AP, hostname) == ESP_OK;
}

/**
 * Enable IPv6 on the softAP interface.
 * @return true on success
 */
bool rpcWiFiAPClass::softAPenableIpV6()
{
    if (rpcWiFiGenericClass::getMode() == WIFI_MODE_NULL)
    {
        return false;
    }
    return new_tcpip_adapter_create_ip6_linklocal(RPC_TCPIP_ADAPTER_IF_AP) == ESP_OK;
}

/**
 * Get the softAP interface IPv6 address.
 * @return IPv6Address softAP IPv6
 */
IPv6Address rpcWiFiAPClass::softAPIPv6()
{
    static ip6_addr_t addr;
    if (rpcWiFiGenericClass::getMode() == WIFI_MODE_NULL)
    {
        return IPv6Address();
    }
    if (new_tcpip_adapter_get_ip6_linklocal(RPC_TCPIP_ADAPTER_IF_AP, &addr))
    {
        return IPv6Address();
    }
    return IPv6Address(addr.addr);
}
