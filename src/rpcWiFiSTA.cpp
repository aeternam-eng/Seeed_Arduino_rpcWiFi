/*
 WiFiSTA.cpp - rpcWiFi library for esp32

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
#include "rpcWiFiSTA.h"

extern "C"
{
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include "new_lwip/err.h"
#include "new_lwip/dns.h"
}

// -----------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------- STA function -----------------------------------------------------
// -----------------------------------------------------------------------------------------------------------------------

bool rpcWiFiSTAClass::_autoReconnect = true;
bool rpcWiFiSTAClass::_useStaticIp = false;

static rpc_wl_status_t _sta_status = RPC_WL_NO_SHIELD;
static EventGroupHandle_t _sta_status_group = NULL;

void rpcWiFiSTAClass::_setStatus(rpc_wl_status_t status)
{
    if (!_sta_status_group)
    {
        _sta_status_group = xEventGroupCreate();
        if (!_sta_status_group)
        {
            log_e("STA Status Group Create Failed!");
            _sta_status = status;
            return;
        }
    }
    xEventGroupClearBits(_sta_status_group, 0x00FFFFFF);
    xEventGroupSetBits(_sta_status_group, status);
}

/**
 * Return Connection status.
 * @return one of the value defined in rpc_wl_status_t
 *
 */
rpc_wl_status_t rpcWiFiSTAClass::status()
{
    if (!_sta_status_group)
    {
        return _sta_status;
    }
    if(wifi_is_connected_to_ap() != RTW_SUCCESS)
    {
        _setStatus(RPC_WL_DISCONNECTED);
    }
    return (rpc_wl_status_t)xEventGroupClearBits(_sta_status_group, 0);
}

/**
 * Start rpcWiFi connection
 * if passphrase is set the most secure supported mode will be automatically selected
 * @param ssid const char*          Pointer to the SSID string.
 * @param passphrase const char *   Optional. Passphrase. Valid characters in a passphrase must be between ASCII 32-126 (decimal).
 * @param bssid uint8_t[6]          Optional. BSSID / MAC of AP
 * @param channel                   Optional. Channel of AP
 * @param connect                   Optional. call connect
 * @return
 */
rpc_wl_status_t rpcWiFiSTAClass::begin(const char *ssid, const char *passphrase, int32_t channel, const uint8_t *bssid, bool connect)
{

    if (!rpcWiFi.enableSTA(true))
    {
        log_e("STA enable failed!");
        return RPC_WL_CONNECT_FAILED;
    }

    if (!ssid || *ssid == 0x00 || strlen(ssid) > 31)
    {
        log_e("SSID too long or missing!");
        return RPC_WL_CONNECT_FAILED;
    }

    if (passphrase && strlen(passphrase) > 64)
    {
        log_e("passphrase too long!");
        return RPC_WL_CONNECT_FAILED;
    }
    int ret = RTW_ERROR;
    uint32_t security_type;
    uint8_t pscan_config;
    security_type = RTW_SECURITY_OPEN;
    if (passphrase != NULL)
    {
        security_type = RTW_SECURITY_WPA2_MIXED_PSK;
    }

    if (channel != 0 && bssid == NULL)
    {
        pscan_config = PSCAN_ENABLE | PSCAN_FAST_SURVEY;
        ret = wifi_set_pscan_chan((uint8_t *)&channel, &pscan_config, 1);
        if (ret < 0)
        {
            wifi_off();
            wifi_on(RTW_MODE_STA);
            return RPC_WL_CONNECT_FAILED;
        }
    }

    if (bssid != NULL)
    {
        ret = wifi_connect_bssid((unsigned char *)bssid, (char *)ssid, security_type, (char *)passphrase, ETH_ALEN, strlen(ssid), strlen(passphrase), -1, NULL);
    }
    else
    {
        ret = wifi_connect((char *)ssid, security_type, (char *)passphrase, strlen(ssid), strlen(passphrase), -1, NULL);
    }

    if (ret == RTW_ERROR)
    {
        log_e("connect failed!");
        return RPC_WL_CONNECT_FAILED;
    }

    if (!_useStaticIp)
    {
        if (new_tcpip_adapter_dhcpc_start(RPC_TCPIP_ADAPTER_IF_STA) == ESP_ERR_TCPIP_ADAPTER_DHCPC_START_FAILED)
        {
            log_e("dhcp client start failed!");
            return RPC_WL_CONNECT_FAILED;
        }
    }
    else
    {
        new_tcpip_adapter_up(RPC_TCPIP_ADAPTER_IF_STA);
    }

    if (connect && (wifi_is_connected_to_ap() != RTW_SUCCESS))
    {
        log_e("connect failed!");
        return RPC_WL_CONNECT_FAILED;
    }

    return status();
}

/**
 * Use to connect to SDK config.
 * @return rpc_wl_status_t
 */
rpc_wl_status_t rpcWiFiSTAClass::begin()
{

    if (!rpcWiFi.enableSTA(true))
    {
        log_e("STA enable failed!");
        return RPC_WL_CONNECT_FAILED;
    }

    wlan_fast_reconnect_profile_t wifi_info = {0};
    if (wifi_get_reconnect_data(&wifi_info) == 0)
    {
        log_e("config failed");
        return RPC_WL_CONNECT_FAILED;
    }

    uint32_t channel;
    uint32_t security_type;
    uint8_t pscan_config;
    char key_id_str[2] = {0};
    int key_id = -1;
    int ret;

    channel = wifi_info.channel;
    sprintf(key_id_str, "%d", (char)(channel >> 28));
    channel &= 0xff;
    security_type = wifi_info.security_type;
    pscan_config = PSCAN_ENABLE | PSCAN_FAST_SURVEY;
    key_id = atoi((const char *)key_id_str);

    // ret = wifi_set_pscan_chan((uint8_t *)&channel, &pscan_config, 1);
    // if (ret < 0)
    // {
    //     log_e("connect failed!");
    //     return RPC_WL_CONNECT_FAILED;
    // }

    if (security_type == RTW_SECURITY_OPEN)
    {
        ret = wifi_connect((char *)wifi_info.psk_essid, security_type, NULL, strlen((char *)wifi_info.psk_essid), 0, key_id, NULL);
    }
    else
    {
        ret = wifi_connect((char *)wifi_info.psk_essid, security_type, (char *)wifi_info.psk_passphrase, strlen((char *)wifi_info.psk_essid), strlen((char *)wifi_info.psk_passphrase), key_id, NULL);
    }

    if (ret == RTW_ERROR)
    {
        log_e("connect failed!");
        return RPC_WL_CONNECT_FAILED;
    }

    if (!_useStaticIp)
    {
        if (new_tcpip_adapter_dhcpc_start(RPC_TCPIP_ADAPTER_IF_STA) == ESP_ERR_TCPIP_ADAPTER_DHCPC_START_FAILED)
        {
            log_e("dhcp client start failed!");
            return RPC_WL_CONNECT_FAILED;
        }
    }
    else
    {
        new_tcpip_adapter_dhcpc_stop(RPC_TCPIP_ADAPTER_IF_STA);
    }

    if (status() != RPC_WL_CONNECTED && (wifi_is_connected_to_ap() != RTW_SUCCESS))
    {
        log_e("connect failed!");
        return RPC_WL_CONNECT_FAILED;
    }

    return status();
}

/**
 * will force a disconnect an then start  reconnectingto AP
 * @return ok
 */
bool rpcWiFiSTAClass::reconnect()
{
    if (rpcWiFi.getMode() & RPC_WIFI_MODE_STA)
    {
        if (wifi_disconnect() == RTW_SUCCESS)
        {
            return begin() != RPC_WL_CONNECT_FAILED;
        }
    }
    return false;
}

/**
 * Disconnect from the network
 * @param wifioff
 * @return  one value of rpc_wl_status_t enum
 */
bool rpcWiFiSTAClass::disconnect(bool wifioff, bool eraseap)
{

    if (rpcWiFi.getMode() & RPC_WIFI_MODE_STA)
    {
        if (eraseap)
        {
            // memset(&conf, 0, sizeof(rpc_wifi_config_t));
            // if(esp_wifi_set_config(WIFI_IF_STA, &conf)){
            //     log_e("clear config failed!");
            // }
        }
        if (wifi_disconnect() != RTW_SUCCESS)
        {
            log_e("disconnect failed!");
            return false;
        }
        if (wifioff)
        {
            return rpcWiFi.enableSTA(false);
        }
        return true;
    }

    return false;
}

/**
 * Change IP configuration settings disabling the dhcp client
 * @param local_ip   Static ip configuration
 * @param gateway    Static gateway configuration
 * @param subnet     Static Subnet mask
 * @param dns1       Static DNS server 1
 * @param dns2       Static DNS server 2
 */
bool rpcWiFiSTAClass::config(IPAddress local_ip, IPAddress gateway, IPAddress subnet, IPAddress dns1, IPAddress dns2)
{
    rpc_esp_err_t err = ESP_OK;

    if (!rpcWiFi.enableSTA(true))
    {
        return false;
    }

    rpc_tcpip_adapter_ip_info_t info;

    if (local_ip != (uint32_t)0x00000000)
    {
        info.ip.addr = static_cast<uint32_t>(local_ip);
        info.gw.addr = static_cast<uint32_t>(gateway);
        info.netmask.addr = static_cast<uint32_t>(subnet);
    }
    else
    {
        info.ip.addr = 0;
        info.gw.addr = 0;
        info.netmask.addr = 0;
    }

    err = new_tcpip_adapter_dhcpc_stop(RPC_TCPIP_ADAPTER_IF_STA);
    if (err != ESP_OK && err != ESP_ERR_TCPIP_ADAPTER_DHCP_ALREADY_STOPPED)
    {
        log_e("DHCP could not be stopped! Error: %d", err);
        return false;
    }

    err = new_tcpip_adapter_set_ip_info(RPC_TCPIP_ADAPTER_IF_STA, &info);
    if (err != ERR_OK)
    {
        log_e("STA IP could not be configured! Error: %d", err);
        return false;
    }

    if (info.ip.addr)
    {
        _useStaticIp = true;
    }
    else
    {
        err = new_tcpip_adapter_dhcpc_start(RPC_TCPIP_ADAPTER_IF_STA);
        if (err == ESP_ERR_TCPIP_ADAPTER_DHCPC_START_FAILED)
        {
            log_e("dhcp client start failed!");
            return false;
        }
        _useStaticIp = false;
    }

    if (dns1 != (uint32_t)0x00000000)
    {
        // Set DNS1-Server
        rpc_tcpip_adapter_dns_info_t tcpip_dns0;
        tcpip_dns0.ipv4 = dns1;
        new_tcpip_adapter_set_dns_info(RPC_TCPIP_ADAPTER_IF_MAX,RPC_TCPIP_ADAPTER_DNS_MAIN,&tcpip_dns0);
    }

    if (dns2 != (uint32_t)0x00000000)
    {
        // Set DNS2-Server
        rpc_tcpip_adapter_dns_info_t tcpip_dns1;
        tcpip_dns1.ipv4 = dns2;
        new_tcpip_adapter_set_dns_info(RPC_TCPIP_ADAPTER_IF_MAX,RPC_TCPIP_ADAPTER_DNS_BACKUP,&tcpip_dns1);
    }

    return true;
}

/**
 * is STA interface connected?
 * @return true if STA is connected to an AD
 */
bool rpcWiFiSTAClass::isConnected()
{
    return (status() == RPC_WL_CONNECTED);
}

/**
 * Clear STA Connected Setting
 * @return true if clear action done
 */
bool rpcWiFiSTAClass::clearConnectedSetting()
{
    if(wifi_clear_reconnect_data() == 0){
        return false;
    }else{
        return true;
    }
}

/**
 * Setting the ESP32 station to connect to the AP (which is recorded)
 * automatically or not when powered on. Enable auto-connect by default.
 * @param autoConnect bool
 * @return if saved
 */
bool rpcWiFiSTAClass::setAutoConnect(bool autoConnect)
{
    // bool ret;
    // ret = (wifi_set_autoreconnect(autoConnect) == 0);
    // return ret;
    return false;
}

/**
 * Checks if ESP32 station mode will connect to AP
 * automatically or not when it is powered on.
 * @return auto connect
 */
bool rpcWiFiSTAClass::getAutoConnect()
{
    // bool autoConnect;
    // uint8_t mode = 0;
    // wifi_get_autoreconnect(&mode);
    // autoConnect = (mode == 1);
    // return autoConnect;
    return false;
}

bool rpcWiFiSTAClass::setAutoReconnect(bool autoReconnect)
{
    // _autoReconnect = autoReconnect;
    // // return true;
    // bool ret;
    // ret = (wifi_set_autoreconnect(autoReconnect) == 0);
    return false;
}

bool rpcWiFiSTAClass::getAutoReconnect()
{
    // //return _autoReconnect;
    // bool autoReconnect;
    // uint8_t mode = 0;
    // wifi_get_autoreconnect(&mode);
    // autoReconnect = (mode == 1);
    return false;
}

/**
 * Wait for rpcWiFi connection to reach a result
 * returns the status reached or disconnect if STA is off
 * @return rpc_wl_status_t
 */
uint8_t rpcWiFiSTAClass::waitForConnectResult()
{
    //1 and 3 have STA enabled
    if ((rpcWiFiGenericClass::getMode() & RPC_WIFI_MODE_STA) == 0)
    {
        return RPC_WL_DISCONNECTED;
    }
    int i = 0;
    while ((!status() || status() >= RPC_WL_DISCONNECTED) && i++ < 100)
    {
        delay(100);
    }
    return status();
}

/**
 * Get the station interface IP address.
 * @return IPAddress station IP
 */
IPAddress rpcWiFiSTAClass::localIP()
{
    if (rpcWiFiGenericClass::getMode() == RPC_WIFI_MODE_NULL)
    {
        return IPAddress();
    }
    rpc_tcpip_adapter_ip_info_t ip;
    new_tcpip_adapter_get_ip_info(RPC_TCPIP_ADAPTER_IF_STA, &ip);
    return IPAddress(ip.ip.addr);
}

/**
 * Get the station interface MAC address.
 * @param mac   pointer to uint8_t array with length WL_MAC_ADDR_LENGTH
 * @return      pointer to uint8_t *
 */
uint8_t *rpcWiFiSTAClass::macAddress(uint8_t *mac)
{
    if (rpcWiFiGenericClass::getMode() != RPC_WIFI_MODE_NULL)
    {
        String macStr = macAddress();
        for (int i = 0; i < 6; i++)
        {
            mac[i] = (uint8_t)htoi(macStr.substring(i * 3, i * 3 + 2).c_str());
        }
    }
    return mac;
}

/**
 * Get the station interface MAC address.
 * @return String mac
 */
String rpcWiFiSTAClass::macAddress(void)
{
    char macStr[18] = {0};
    if (rpcWiFiGenericClass::getMode() == RPC_WIFI_MODE_NULL)
    {
        return String();
    }
    new_tcpip_adapter_get_mac(RPC_TCPIP_ADAPTER_IF_AP, (uint8_t *)macStr);

    return String(macStr);
}

/**
 * Get the interface subnet mask address.
 * @return IPAddress subnetMask
 */
IPAddress rpcWiFiSTAClass::subnetMask()
{
    if (rpcWiFiGenericClass::getMode() == RPC_WIFI_MODE_NULL)
    {
        return IPAddress();
    }
    rpc_tcpip_adapter_ip_info_t ip;
    new_tcpip_adapter_get_ip_info(RPC_TCPIP_ADAPTER_IF_STA, &ip);
    return IPAddress(ip.netmask.addr);
}

/**
 * Get the gateway ip address.
 * @return IPAddress gatewayIP
 */
IPAddress rpcWiFiSTAClass::gatewayIP()
{
    if (rpcWiFiGenericClass::getMode() == RPC_WIFI_MODE_NULL)
    {
        return IPAddress();
    }
    rpc_tcpip_adapter_ip_info_t ip;
    new_tcpip_adapter_get_ip_info(RPC_TCPIP_ADAPTER_IF_STA, &ip);
    return IPAddress(ip.gw.addr);
}

/**
 * Get the DNS ip address.
 * @param dns_no
 * @return IPAddress DNS Server IP
 */
IPAddress rpcWiFiSTAClass::dnsIP(uint8_t dns_no)
{
    if (rpcWiFiGenericClass::getMode() == RPC_WIFI_MODE_NULL)
    {
        return IPAddress();
    }

    rpc_tcpip_adapter_dns_info_t tcpip_dns_ip;

    if(dns_no == RPC_TCPIP_ADAPTER_DNS_MAIN){
        new_tcpip_adapter_get_dns_info(RPC_TCPIP_ADAPTER_IF_MAX,RPC_TCPIP_ADAPTER_DNS_MAIN,&tcpip_dns_ip);
        return IPAddress(tcpip_dns_ip.ipv4);
    }else{
        new_tcpip_adapter_get_dns_info(RPC_TCPIP_ADAPTER_IF_MAX,RPC_TCPIP_ADAPTER_DNS_BACKUP,&tcpip_dns_ip);
        return IPAddress(tcpip_dns_ip.ipv4);
    }
}

/**
 * Get the broadcast ip address.
 * @return IPAddress broadcastIP
 */
IPAddress rpcWiFiSTAClass::broadcastIP()
{
    if (rpcWiFiGenericClass::getMode() == RPC_WIFI_MODE_NULL)
    {
        return IPAddress();
    }
    rpc_tcpip_adapter_ip_info_t ip;
    new_tcpip_adapter_get_ip_info(RPC_TCPIP_ADAPTER_IF_STA, &ip);
    return rpcWiFiGenericClass::calculateBroadcast(IPAddress(ip.gw.addr), IPAddress(ip.netmask.addr));
}

/**
 * Get the network id.
 * @return IPAddress networkID
 */
IPAddress rpcWiFiSTAClass::networkID()
{
    if (rpcWiFiGenericClass::getMode() == RPC_WIFI_MODE_NULL)
    {
        return IPAddress();
    }
    rpc_tcpip_adapter_ip_info_t ip;
    new_tcpip_adapter_get_ip_info(RPC_TCPIP_ADAPTER_IF_STA, &ip);
    return rpcWiFiGenericClass::calculateNetworkID(IPAddress(ip.gw.addr), IPAddress(ip.netmask.addr));
}

/**
 * Get the subnet CIDR.
 * @return uint8_t subnetCIDR
 */
uint8_t rpcWiFiSTAClass::subnetCIDR()
{
    if (rpcWiFiGenericClass::getMode() == RPC_WIFI_MODE_NULL)
    {
        return (uint8_t)0;
    }
    rpc_tcpip_adapter_ip_info_t ip;
    new_tcpip_adapter_get_ip_info(RPC_TCPIP_ADAPTER_IF_STA, &ip);
    return rpcWiFiGenericClass::calculateSubnetCIDR(IPAddress(ip.netmask.addr));
}

/**
 * Return the current SSID associated with the network
 * @return SSID
 */
String rpcWiFiSTAClass::SSID() const
{  
    if(rpcWiFiGenericClass::getMode() == RPC_WIFI_MODE_APSTA)
    {
        rtw_wifi_setting_t wifi_setting;
        wifi_get_setting(WLAN1_NAME,&wifi_setting);
        return String(reinterpret_cast<char *>(wifi_setting.ssid));
    }
    else if(rpcWiFiGenericClass::getMode() == RPC_WIFI_MODE_STA)
    {
        rtw_wifi_setting_t wifi_setting;
        wifi_get_setting(WLAN0_NAME,&wifi_setting);
        return String(reinterpret_cast<char *>(wifi_setting.ssid));
    }

    return String();
}

/**
 * Return the current pre shared key associated with the network
 * @return  psk string
 */
String rpcWiFiSTAClass::psk() const
{
    if (rpcWiFiGenericClass::getMode() == RPC_WIFI_MODE_NULL)
    {
        return String();
    }
    rpc_wifi_config_t conf;
    esp_wifi_get_config(WIFI_IF_STA, &conf);
    return String(reinterpret_cast<char *>(conf.sta.password));
}

/**
 * Return the current bssid / mac associated with the network if configured
 * @return bssid uint8_t *
 */
uint8_t *rpcWiFiSTAClass::BSSID(void)
{
    static uint8_t bssid[6];
    if (rpcWiFiGenericClass::getMode() == RPC_WIFI_MODE_NULL)
    {
        return NULL;
    }
    if (!wifi_get_ap_bssid(bssid))
    {
        return reinterpret_cast<uint8_t *>(bssid);
    }
    return NULL;
}

/**
 * Return the current bssid / mac associated with the network if configured
 * @return String bssid mac
 */
String rpcWiFiSTAClass::BSSIDstr(void)
{
    uint8_t *bssid = BSSID();
    if (!bssid)
    {
        return String();
    }
    char mac[18] = {0};
    sprintf(mac, "%02X:%02X:%02X:%02X:%02X:%02X", bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5]);
    return String(mac);
}

/**
 * Return the current network RSSI.
 * @return  RSSI value
 */
int8_t rpcWiFiSTAClass::RSSI(void)
{
    if (rpcWiFiGenericClass::getMode() == RPC_WIFI_MODE_NULL)
    {
        return 0;
    }
    int rssi = 0;
    if (wifi_get_rssi(&rssi) == RTW_SUCCESS)
    {
        return rssi;
    }
    return 0;
}

/**
 * Get the station interface Host name.
 * @return char array hostname
 */
const char *rpcWiFiSTAClass::getHostname()
{
    const char *hostname = NULL;
    if (rpcWiFiGenericClass::getMode() == RPC_WIFI_MODE_NULL)
    {
        return hostname;
    }
    if (new_tcpip_adapter_get_hostname(RPC_TCPIP_ADAPTER_IF_STA, &hostname))
    {
        return NULL;
    }
    return hostname;
}

/**
 * Set the station interface Host name.
 * @param  hostname  pointer to const string
 * @return true on   success
 */
bool rpcWiFiSTAClass::setHostname(const char *hostname)
{
    if (rpcWiFiGenericClass::getMode() == RPC_WIFI_MODE_NULL)
    {
        return false;
    }
    return new_tcpip_adapter_set_hostname(RPC_TCPIP_ADAPTER_IF_STA, hostname) == 0;
}

/**
 * Enable IPv6 on the station interface.
 * @return true on success
 */
bool rpcWiFiSTAClass::enableIpV6()
{
    if (rpcWiFiGenericClass::getMode() == RPC_WIFI_MODE_NULL)
    {
        return false;
    }
    return new_tcpip_adapter_create_ip6_linklocal(RPC_TCPIP_ADAPTER_IF_STA) == 0;
}

/**
 * Get the station interface IPv6 address.
 * @return IPv6Address
 */
IPv6Address rpcWiFiSTAClass::localIPv6()
{
    static ip6_addr_t addr;
    if (rpcWiFiGenericClass::getMode() == RPC_WIFI_MODE_NULL)
    {
        return IPv6Address();
    }
    if (new_tcpip_adapter_get_ip6_linklocal(RPC_TCPIP_ADAPTER_IF_STA, &addr))
    {
        return IPv6Address();
    }
    return IPv6Address(addr.addr);
}

// bool rpcWiFiSTAClass::_smartConfigStarted = false;
// bool rpcWiFiSTAClass::_smartConfigDone = false;

// bool rpcWiFiSTAClass::beginSmartConfig() {
//     if (_smartConfigStarted) {
//         return false;
//     }

//     if (!rpcWiFi.mode(rpcWIFI_STA)) {
//         return false;
//     }

//     esp_wifi_disconnect();

//     esp_err_t err;
//     err = esp_smartconfig_start(reinterpret_cast<sc_callback_t>(&rpcWiFiSTAClass::_smartConfigCallback), 1);
//     if (err == ESP_OK) {
//         _smartConfigStarted = true;
//         _smartConfigDone = false;
//         return true;
//     }
//     return false;
// }

// bool rpcWiFiSTAClass::stopSmartConfig() {
//     if (!_smartConfigStarted) {
//         return true;
//     }

//     if (esp_smartconfig_stop() == ESP_OK) {
//         _smartConfigStarted = false;
//         return true;
//     }

//     return false;
// }

// bool rpcWiFiSTAClass::smartConfigDone() {
//     if (!_smartConfigStarted) {
//         return false;
//     }

//     return _smartConfigDone;
// }

// #if ARDUHAL_LOG_LEVEL >= ARDUHAL_LOG_LEVEL_DEBUG
// const char * sc_status_strings[] = {
//     "WAIT",
//     "FIND_CHANNEL",
//     "GETTING_SSID_PSWD",
//     "LINK",
//     "LINK_OVER"
// };

// const char * sc_type_strings[] = {
//     "ESPTOUCH",
//     "AIRKISS",
//     "ESPTOUCH_AIRKISS"
// };
// #endif

// void rpcWiFiSTAClass::_smartConfigCallback(uint32_t st, void* result) {
//     smartconfig_status_t status = (smartconfig_status_t) st;
//     log_d("Status: %s", sc_status_strings[st % 5]);
//     if (status == SC_STATUS_GETTING_SSID_PSWD) {
// #if ARDUHAL_LOG_LEVEL >= ARDUHAL_LOG_LEVEL_DEBUG
//         smartconfig_type_t * type = (smartconfig_type_t *)result;
//         log_d("Type: %s", sc_type_strings[*type % 3]);
// #endif
//     } else if (status == SC_STATUS_LINK) {
//         rpc_wifi_sta_config_t *sta_conf = reinterpret_cast<rpc_wifi_sta_config_t *>(result);
//         log_d("SSID: %s", (char *)(sta_conf->ssid));
//         sta_conf->bssid_set = 0;
//         esp_wifi_set_config(WIFI_IF_STA, (rpc_wifi_config_t *)sta_conf);
//         esp_wifi_connect();
//         _smartConfigDone = true;
//     } else if (status == SC_STATUS_LINK_OVER) {
//         if(result){
// #if ARDUHAL_LOG_LEVEL >= ARDUHAL_LOG_LEVEL_DEBUG
//             new_ip4_addr_t * ip = (new_ip4_addr_t *)result;
//             log_d("Sender IP: " IPSTR, IP2STR(ip));
// #endif
//         }
//         rpcWiFi.stopSmartConfig();
//     }
// }
