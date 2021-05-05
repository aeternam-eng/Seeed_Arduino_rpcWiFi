/*
 ESP8266WiFiGeneric.h - esp8266 Wifi support.
 Based on WiFi.h from Ardiono WiFi shield library.
 Copyright (c) 2011-2014 Arduino.  All right reserved.
 Modified by Ivan Grokhotkov, December 2014
 Reworked by Markus Sattler, December 2015

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
 */

#ifndef RPCESP32WIFIGENERIC_H_
#define RPCESP32WIFIGENERIC_H_

#include "seeed_rpcUnified.h"
#include "rtl_wifi/wifi_unified.h"
#undef max
#undef min
#include <functional>
#include "WiFiType.h"

int htoi(const char *s);

typedef void (*rpcWiFiEventCb)(rpc_system_event_id_t event);
typedef std::function<void(rpc_system_event_id_t event, rpc_system_event_info_t info)> rpcWiFiEventFuncCb;
typedef void (*rpcWiFiEventSysCb)(rpc_system_event_t *event);

typedef size_t rpc_wifi_event_id_t;

typedef enum  {
    RPC_WIFI_POWER_19_5dBm = 78,// 19.5dBm
    RPC_WIFI_POWER_19dBm = 76,// 19dBm
    RPC_WIFI_POWER_18_5dBm = 74,// 18.5dBm
    RPC_WIFI_POWER_17dBm = 68,// 17dBm
    RPC_WIFI_POWER_15dBm = 60,// 15dBm
    RPC_WIFI_POWER_13dBm = 52,// 13dBm
    RPC_WIFI_POWER_11dBm = 44,// 11dBm
    RPC_WIFI_POWER_8_5dBm = 34,// 8.5dBm
    RPC_WIFI_POWER_7dBm = 28,// 7dBm
    RPC_WIFI_POWER_5dBm = 20,// 5dBm
    RPC_WIFI_POWER_2dBm = 8,// 2dBm
    RPC_WIFI_POWER_MINUS_1dBm = -4// -1dBm
} rpc_wifi_power_t;

static const int RPC_AP_STARTED_BIT    = BIT0;
static const int RPC_AP_HAS_IP6_BIT    = BIT1;
static const int RPC_AP_HAS_CLIENT_BIT = BIT2;
static const int RPC_STA_STARTED_BIT   = BIT3;
static const int RPC_STA_CONNECTED_BIT = BIT4;
static const int RPC_STA_HAS_IP_BIT    = BIT5;
static const int RPC_STA_HAS_IP6_BIT   = BIT6;
static const int RPC_ETH_STARTED_BIT   = BIT7;
static const int RPC_ETH_CONNECTED_BIT = BIT8;
static const int RPC_ETH_HAS_IP_BIT    = BIT9;
static const int RPC_ETH_HAS_IP6_BIT   = BIT10;
static const int RPC_WIFI_SCANNING_BIT = BIT11;
static const int RPC_WIFI_SCAN_DONE_BIT= BIT12;
static const int RPC_WIFI_DNS_IDLE_BIT = BIT13;
static const int RPC_WIFI_DNS_DONE_BIT = BIT14;

class rpcWiFiGenericClass
{
  public:
    rpcWiFiGenericClass();

    rpc_wifi_event_id_t onEvent(rpcWiFiEventCb cbEvent, rpc_system_event_id_t event = RPC_SYSTEM_EVENT_MAX);
    rpc_wifi_event_id_t onEvent(rpcWiFiEventFuncCb cbEvent, rpc_system_event_id_t event = RPC_SYSTEM_EVENT_MAX);
    rpc_wifi_event_id_t onEvent(rpcWiFiEventSysCb cbEvent, rpc_system_event_id_t event = RPC_SYSTEM_EVENT_MAX);
    void removeEvent(rpcWiFiEventCb cbEvent, rpc_system_event_id_t event = RPC_SYSTEM_EVENT_MAX);
    void removeEvent(rpcWiFiEventSysCb cbEvent, rpc_system_event_id_t event = RPC_SYSTEM_EVENT_MAX);
    void removeEvent(rpc_wifi_event_id_t id);

    static int getStatusBits();
    static int waitStatusBits(int bits, uint32_t timeout_ms);

    int32_t channel(void);

    void persistent(bool persistent);

    static bool mode(rpc_wifi_mode_t);
    static rpc_wifi_mode_t getMode();

    bool enableSTA(bool enable);
    bool enableAP(bool enable);

    bool setSleep(bool enable);
    bool getSleep();

    bool setTxPower(rpc_wifi_power_t power);
    rpc_wifi_power_t getTxPower();

    static rpc_esp_err_t _eventCallback(void *arg, rpc_system_event_t *event);

  protected:
    static bool _persistent;
    static rpc_wifi_mode_t _wifi_mode;
    static rpc_wifi_power_t _wifi_power;
    static rpc_wifi_mode_t _forceSleepLastMode;

    static int setStatusBits(int bits);
    static int clearStatusBits(int bits);

  public:
    static int hostByName(const char *aHostname, IPAddress &aResult);

    static IPAddress calculateNetworkID(IPAddress ip, IPAddress subnet);
    static IPAddress calculateBroadcast(IPAddress ip, IPAddress subnet);
    static uint8_t calculateSubnetCIDR(IPAddress subnetMask);

  protected:
    friend class rpcWiFiSTAClass;
    friend class rpcWiFiScanClass;
    friend class rpcWiFiAPClass;
};

#endif /* ESP32WIFIGENERIC_H_ */