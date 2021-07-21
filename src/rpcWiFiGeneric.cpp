/*
 ESP8266WiFiGeneric.cpp - rpcWiFi library for esp8266

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

extern "C"
{
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include "new_lwip/ip_addr.h"
#include "new_lwip/opt.h"
#include "new_lwip/err.h"
#include "new_lwip/dns.h"
} //extern "C"

#include <vector>

// static xQueueHandle _network_event_queue;
// static TaskHandle_t _network_event_task_handle = NULL;
static EventGroupHandle_t _network_event_group = NULL;

rpc_wifi_mode_t rpcWiFiGenericClass::_wifi_mode = RPC_WIFI_MODE_NULL;
rpc_wifi_power_t rpcWiFiGenericClass::_wifi_power = RPC_WIFI_POWER_19_5dBm;

// static void _network_event_task(void * arg){
//     rpc_system_event_t *event = NULL;
//     for (;;) {
//         if(xQueueReceive(_network_event_queue, &event, portMAX_DELAY) == pdTRUE){
//             WiFiGenericClass::_eventCallback(arg, event);
//         }
//     }
//     vTaskDelete(NULL);
//     _network_event_task_handle = NULL;
// }

// static esp_err_t _network_event_cb(void *arg, rpc_system_event_t *event){
//     if (xQueueSend(_network_event_queue, &event, portMAX_DELAY) != pdPASS) {
//         log_w("Network Event Queue Send Failed!");
//         return ESP_FAIL;
//     }
//     return ESP_OK;
// }

// static bool _start_network_event_task(){
//     if(!_network_event_group){
//         _network_event_group = xEventGroupCreate();
//         if(!_network_event_group){
//             log_e("Network Event Group Create Failed!");
//             return false;
//         }
//         xEventGroupSetBits(_network_event_group, RPC_WIFI_DNS_IDLE_BIT);
//     }
//     if(!_network_event_queue){
//         _network_event_queue = xQueueCreate(32, sizeof(rpc_system_event_t *));
//         if(!_network_event_queue){
//             log_e("Network Event Queue Create Failed!");
//             return false;
//         }
//     }
//     if(!_network_event_task_handle){
//         xTaskCreateUniversal(_network_event_task, "network_event", 4096, NULL, ESP_TASKD_EVENT_PRIO - 1, &_network_event_task_handle, CONFIG_ARDUINO_EVENT_RUNNING_CORE);
//         if(!_network_event_task_handle){
//             log_e("Network Event Task Start Failed!");
//             return false;
//         }
//     }
//     //TODO
//     return true;
//     //return rpc_esp_event_loop_init(&_network_event_cb, NULL) == ESP_OK;
// }

int htoi(const char *s)
{
    int i;
    int n = 0;
    if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))
    {
        i = 2;
    }
    else
    {
        i = 0;
    }
    for (; (s[i] >= '0' && s[i] <= '9') || (s[i] >= 'a' && s[i] <= 'z') || (s[i] >= 'A' && s[i] <= 'Z'); ++i)
    {
        if (tolower(s[i]) > '9')
        {
            n = 16 * n + (10 + tolower(s[i]) - 'a');
        }
        else
        {
            n = 16 * n + (tolower(s[i]) - '0');
        }
    }
    return n;
}

void new_tcpipInit()
{
    static bool initialized = false;
    if (!initialized)
    {
        initialized = true;
    }
}

static bool lowLevelInitDone = false;
static bool wifiLowLevelInit(bool persistent)
{
    if (!lowLevelInitDone)
    {
        if (!_network_event_group)
        {
            _network_event_group = xEventGroupCreate();
            if (!_network_event_group)
            {
                log_e("Network Event Group Create Failed!");
                return false;
            }
            xEventGroupSetBits(_network_event_group, RPC_WIFI_DNS_IDLE_BIT);
        }
        new_tcpip_adapter_init();
        system_event_callback_reg(rpcWiFiGenericClass::_eventCallback);
        lowLevelInitDone = true;
    }
    return true;
}

static bool wifiLowLevelDeinit()
{
    //deinit not working yet!
    wifi_off();
    new_tcpip_adapter_stop(RPC_TCPIP_ADAPTER_IF_STA);
    new_tcpip_adapter_stop(RPC_TCPIP_ADAPTER_IF_AP);

    return true;
}

static bool _esp_wifi_started = false;

static bool espWiFiStart(bool persistent)
{
    if (_esp_wifi_started)
    {
        return true;
    }
    if (!wifiLowLevelInit(persistent))
    {
        return false;
    }

    _esp_wifi_started = true;
    rpc_system_event_t event;
    event.event_id = RPC_SYSTEM_EVENT_WIFI_READY;
    rpcWiFiGenericClass::_eventCallback(nullptr, &event);

    return true;
}

static bool espWiFiStop()
{

    if (!_esp_wifi_started)
    {
        return true;
    }
    _esp_wifi_started = false;

    return wifiLowLevelDeinit();
}

// -----------------------------------------------------------------------------------------------------------------------
// ------------------------------------------------- Generic rpcWiFi function -----------------------------------------------
// -----------------------------------------------------------------------------------------------------------------------

typedef struct rpcWiFiEventCbList
{
    static rpc_wifi_event_id_t current_id;
    rpc_wifi_event_id_t id;
    rpcWiFiEventCb cb;
    rpcWiFiEventFuncCb fcb;
    rpcWiFiEventSysCb scb;
    rpc_system_event_id_t event;

    rpcWiFiEventCbList() : id(current_id++), cb(NULL), fcb(NULL), scb(NULL), event(RPC_SYSTEM_EVENT_WIFI_READY) {}
} rpcWiFiEventCbList_t;
rpc_wifi_event_id_t rpcWiFiEventCbList::current_id = 1;

// arduino dont like std::vectors move static here
static std::vector<rpcWiFiEventCbList_t> cbEventList;

bool rpcWiFiGenericClass::_persistent = true;
rpc_wifi_mode_t rpcWiFiGenericClass::_forceSleepLastMode = RPC_WIFI_MODE_NULL;

rpcWiFiGenericClass::rpcWiFiGenericClass()
{
}

int rpcWiFiGenericClass::setStatusBits(int bits)
{
    if (!_network_event_group)
    {
        return 0;
    }
    return xEventGroupSetBits(_network_event_group, bits);
}

int rpcWiFiGenericClass::clearStatusBits(int bits)
{
    if (!_network_event_group)
    {
        return 0;
    }
    return xEventGroupClearBits(_network_event_group, bits);
}

int rpcWiFiGenericClass::getStatusBits()
{
    if (!_network_event_group)
    {
        return 0;
    }
    return xEventGroupGetBits(_network_event_group);
}

int rpcWiFiGenericClass::waitStatusBits(int bits, uint32_t timeout_ms)
{
    if (!_network_event_group)
    {
        return 0;
    }
    return xEventGroupWaitBits(
               _network_event_group, // The event group being tested.
               bits,                 // The bits within the event group to wait for.
               pdFALSE,              // BIT_0 and BIT_4 should be cleared before returning.
               pdTRUE,               // Don't wait for both bits, either bit will do.
               timeout_ms / portTICK_PERIOD_MS) &
           bits; // Wait a maximum of 100ms for either bit to be set.
}

/**
 * set callback function
 * @param cbEvent rpcWiFiEventCb
 * @param event optional filter (WIFI_EVENT_MAX is all events)
 */
rpc_wifi_event_id_t rpcWiFiGenericClass::onEvent(rpcWiFiEventCb cbEvent, rpc_system_event_id_t event)
{
    if (!cbEvent)
    {
        return 0;
    }
    rpcWiFiEventCbList_t newEventHandler;
    newEventHandler.cb = cbEvent;
    newEventHandler.fcb = NULL;
    newEventHandler.scb = NULL;
    newEventHandler.event = event;
    cbEventList.push_back(newEventHandler);
    return newEventHandler.id;
}

rpc_wifi_event_id_t rpcWiFiGenericClass::onEvent(rpcWiFiEventFuncCb cbEvent, rpc_system_event_id_t event)
{
    if (!cbEvent)
    {
        return 0;
    }
    rpcWiFiEventCbList_t newEventHandler;
    newEventHandler.cb = NULL;
    newEventHandler.fcb = cbEvent;
    newEventHandler.scb = NULL;
    newEventHandler.event = event;
    cbEventList.push_back(newEventHandler);
    return newEventHandler.id;
}

rpc_wifi_event_id_t rpcWiFiGenericClass::onEvent(rpcWiFiEventSysCb cbEvent, rpc_system_event_id_t event)
{
    if (!cbEvent)
    {
        return 0;
    }
    rpcWiFiEventCbList_t newEventHandler;
    newEventHandler.cb = NULL;
    newEventHandler.fcb = NULL;
    newEventHandler.scb = cbEvent;
    newEventHandler.event = event;
    cbEventList.push_back(newEventHandler);
    return newEventHandler.id;
}

/**
 * removes a callback form event handler
 * @param cbEvent rpcWiFiEventCb
 * @param event optional filter (WIFI_EVENT_MAX is all events)
 */
void rpcWiFiGenericClass::removeEvent(rpcWiFiEventCb cbEvent, rpc_system_event_id_t event)
{
    if (!cbEvent)
    {
        return;
    }

    for (uint32_t i = 0; i < cbEventList.size(); i++)
    {
        rpcWiFiEventCbList_t entry = cbEventList[i];
        if (entry.cb == cbEvent && entry.event == event)
        {
            cbEventList.erase(cbEventList.begin() + i);
        }
    }
}

void rpcWiFiGenericClass::removeEvent(rpcWiFiEventSysCb cbEvent, rpc_system_event_id_t event)
{
    if (!cbEvent)
    {
        return;
    }

    for (uint32_t i = 0; i < cbEventList.size(); i++)
    {
        rpcWiFiEventCbList_t entry = cbEventList[i];
        if (entry.scb == cbEvent && entry.event == event)
        {
            cbEventList.erase(cbEventList.begin() + i);
        }
    }
}

void rpcWiFiGenericClass::removeEvent(rpc_wifi_event_id_t id)
{
    for (uint32_t i = 0; i < cbEventList.size(); i++)
    {
        rpcWiFiEventCbList_t entry = cbEventList[i];
        if (entry.id == id)
        {
            cbEventList.erase(cbEventList.begin() + i);
        }
    }
}

/**
 * callback for rpcWiFi events
 * @param arg
 */
#if ARDUHAL_LOG_LEVEL >= ARDUHAL_LOG_LEVEL_DEBUG
const char *rpc_system_event_names[] = {"WIFI_READY", "SCAN_DONE", "STA_START", "STA_STOP", "STA_CONNECTED", "STA_DISCONNECTED", "STA_AUTHMODE_CHANGE", "STA_GOT_IP", "STA_LOST_IP", "STA_WPS_ER_SUCCESS", "STA_WPS_ER_FAILED", "STA_WPS_ER_TIMEOUT", "STA_WPS_ER_PIN", "STA_WPS_OVERLAP", "AP_START", "AP_STOP", "AP_STACONNECTED", "AP_STADISCONNECTED", "AP_STAIPASSIGNED", "AP_PROBEREQRECVED", "GOT_IP6", "ETH_START", "ETH_STOP", "ETH_CONNECTED", "ETH_DISCONNECTED", "ETH_GOT_IP", "MAX"};
#endif
#if ARDUHAL_LOG_LEVEL >= ARDUHAL_LOG_LEVEL_WARN
const char *rpc_system_event_reasons[] = {"UNSPECIFIED", "AUTH_EXPIRE", "AUTH_LEAVE", "ASSOC_EXPIRE", "ASSOC_TOOMANY", "NOT_AUTHED", "NOT_ASSOCED", "ASSOC_LEAVE", "ASSOC_NOT_AUTHED", "DISASSOC_PWRCAP_BAD", "DISASSOC_SUPCHAN_BAD", "UNSPECIFIED", "IE_INVALID", "MIC_FAILURE", "4WAY_HANDSHAKE_TIMEOUT", "GROUP_KEY_UPDATE_TIMEOUT", "IE_IN_4WAY_DIFFERS", "GROUP_CIPHER_INVALID", "PAIRWISE_CIPHER_INVALID", "AKMP_INVALID", "UNSUPP_RSN_IE_VERSION", "INVALID_RSN_IE_CAP", "802_1X_AUTH_FAILED", "CIPHER_SUITE_REJECTED", "BEACON_TIMEOUT", "NO_AP_FOUND", "AUTH_FAIL", "ASSOC_FAIL", "HANDSHAKE_TIMEOUT"};
#define reason2str(r) ((r > 176) ? rpc_system_event_reasons[r - 176] : rpc_system_event_reasons[r - 1])
#endif
rpc_esp_err_t rpcWiFiGenericClass::_eventCallback(void *arg, rpc_system_event_t *event)
{
    log_d("rpcWiFi Event: %d", event->event_id);
    if (event->event_id < 27)
    {
        log_d("Event: %d - %s", event->event_id, rpc_system_event_names[event->event_id]);
    }
    if (event->event_id == RPC_SYSTEM_EVENT_SCAN_DONE)
    {
        rpcWiFiScanClass::_scanDone();
    }
    else if (event->event_id == RPC_SYSTEM_EVENT_STA_START)
    {
        rpcWiFiSTAClass::_setStatus(RPC_WL_DISCONNECTED);
        setStatusBits(RPC_STA_STARTED_BIT);
    }
    else if (event->event_id == RPC_SYSTEM_EVENT_STA_STOP)
    {
        rpcWiFiSTAClass::_setStatus(RPC_WL_NO_SHIELD);
        clearStatusBits(RPC_STA_STARTED_BIT | RPC_STA_CONNECTED_BIT | RPC_STA_HAS_IP_BIT | RPC_STA_HAS_IP6_BIT);
    }
    else if (event->event_id == RPC_SYSTEM_EVENT_STA_CONNECTED)
    {
        if(rpcWiFiSTAClass::_useStaticIp){
            rpcWiFiSTAClass::_setStatus(RPC_WL_CONNECTED);
            setStatusBits(RPC_STA_HAS_IP_BIT | RPC_STA_CONNECTED_BIT);
        } else {
            rpcWiFiSTAClass::_setStatus(RPC_WL_IDLE_STATUS);
            setStatusBits(RPC_STA_CONNECTED_BIT);
        }
    }
    else if (event->event_id == RPC_SYSTEM_EVENT_STA_DISCONNECTED)
    {
        uint8_t reason = event->event_info.disconnected.reason;
        // log_w("Reason: %u - %s", reason, reason2str(reason));
        if (reason == RPC_WIFI_REASON_NO_AP_FOUND)
        {
            rpcWiFiSTAClass::_setStatus(RPC_WL_NO_SSID_AVAIL);
        }
        else if (reason == RPC_WIFI_REASON_AUTH_FAIL || reason == RPC_WIFI_REASON_ASSOC_FAIL)
        {
            rpcWiFiSTAClass::_setStatus(RPC_WL_CONNECT_FAILED);
        }
        else if (reason == RPC_WIFI_REASON_BEACON_TIMEOUT || reason == RPC_WIFI_REASON_HANDSHAKE_TIMEOUT)
        {
            rpcWiFiSTAClass::_setStatus(RPC_WL_CONNECTION_LOST);
        }
        else if (reason == RPC_WIFI_REASON_AUTH_EXPIRE)
        {
        }
        else
        {
            rpcWiFiSTAClass::_setStatus(RPC_WL_DISCONNECTED);
        }
        clearStatusBits(RPC_STA_CONNECTED_BIT | RPC_STA_HAS_IP_BIT | RPC_STA_HAS_IP6_BIT);
        if (((reason == RPC_WIFI_REASON_AUTH_EXPIRE) ||
             (reason >= RPC_WIFI_REASON_BEACON_TIMEOUT && reason != RPC_WIFI_REASON_AUTH_FAIL)) &&
            rpcWiFi.getAutoReconnect())
        {
            rpcWiFi.disconnect();
            rpcWiFi.begin();
        }
    }
    else if (event->event_id == RPC_SYSTEM_EVENT_STA_GOT_IP)
    {
#if ARDUHAL_LOG_LEVEL >= ARDUHAL_LOG_LEVEL_DEBUG
        uint8_t *ip = (uint8_t *)&(event->event_info.got_ip.ip_info.ip.addr);
        uint8_t *mask = (uint8_t *)&(event->event_info.got_ip.ip_info.netmask.addr);
        uint8_t *gw = (uint8_t *)&(event->event_info.got_ip.ip_info.gw.addr);
        log_d("STA IP: %u.%u.%u.%u, MASK: %u.%u.%u.%u, GW: %u.%u.%u.%u",
              ip[0], ip[1], ip[2], ip[3],
              mask[0], mask[1], mask[2], mask[3],
              gw[0], gw[1], gw[2], gw[3]);
#endif
        rpcWiFiSTAClass::_setStatus(RPC_WL_CONNECTED);
        setStatusBits(RPC_STA_HAS_IP_BIT | RPC_STA_CONNECTED_BIT);
    }
    else if (event->event_id == RPC_SYSTEM_EVENT_STA_LOST_IP)
    {
        rpcWiFiSTAClass::_setStatus(RPC_WL_IDLE_STATUS);
        clearStatusBits(RPC_STA_HAS_IP_BIT);
    }
    else if (event->event_id == RPC_SYSTEM_EVENT_AP_START)
    {
        setStatusBits(RPC_AP_STARTED_BIT);
    }
    else if (event->event_id == RPC_SYSTEM_EVENT_AP_STOP)
    {
        clearStatusBits(RPC_AP_STARTED_BIT | RPC_AP_HAS_CLIENT_BIT);
    }
    else if (event->event_id == RPC_SYSTEM_EVENT_AP_STACONNECTED)
    {
        setStatusBits(RPC_AP_HAS_CLIENT_BIT);
    }
    else if (event->event_id == RPC_SYSTEM_EVENT_AP_STADISCONNECTED)
    {
        //rpc_wifi_sta_list_t clients;
        if (rpcWiFi.softAPgetStationNum() == 0)
        {
            clearStatusBits(RPC_AP_HAS_CLIENT_BIT);
        }
    }
    else if (event->event_id == RPC_SYSTEM_EVENT_ETH_START)
    {
        setStatusBits(RPC_ETH_STARTED_BIT);
    }
    else if (event->event_id == RPC_SYSTEM_EVENT_ETH_STOP)
    {
        clearStatusBits(RPC_ETH_STARTED_BIT | RPC_ETH_CONNECTED_BIT | RPC_ETH_HAS_IP_BIT | RPC_ETH_HAS_IP6_BIT);
    }
    else if (event->event_id == RPC_SYSTEM_EVENT_ETH_CONNECTED)
    {
        setStatusBits(RPC_ETH_CONNECTED_BIT);
    }
    else if (event->event_id == RPC_SYSTEM_EVENT_ETH_DISCONNECTED)
    {
        clearStatusBits(RPC_ETH_CONNECTED_BIT | RPC_ETH_HAS_IP_BIT | RPC_ETH_HAS_IP6_BIT);
    }
    else if (event->event_id == RPC_SYSTEM_EVENT_ETH_GOT_IP)
    {
#if ARDUHAL_LOG_LEVEL >= ARDUHAL_LOG_LEVEL_DEBUG
        uint8_t *ip = (uint8_t *)&(event->event_info.got_ip.ip_info.ip.addr);
        uint8_t *mask = (uint8_t *)&(event->event_info.got_ip.ip_info.netmask.addr);
        uint8_t *gw = (uint8_t *)&(event->event_info.got_ip.ip_info.gw.addr);
        log_d("ETH IP: %u.%u.%u.%u, MASK: %u.%u.%u.%u, GW: %u.%u.%u.%u",
              ip[0], ip[1], ip[2], ip[3],
              mask[0], mask[1], mask[2], mask[3],
              gw[0], gw[1], gw[2], gw[3]);
#endif
        setStatusBits(RPC_ETH_CONNECTED_BIT | RPC_ETH_HAS_IP_BIT);
    }
    else if (event->event_id == RPC_SYSTEM_EVENT_GOT_IP6)
    {
        if (event->event_info.got_ip6.if_index == RPC_TCPIP_ADAPTER_IF_AP)
        {
            setStatusBits(RPC_AP_HAS_IP6_BIT);
        }
        else if (event->event_info.got_ip6.if_index == RPC_TCPIP_ADAPTER_IF_STA)
        {
            setStatusBits(RPC_STA_CONNECTED_BIT | RPC_STA_HAS_IP6_BIT);
        }
        else if (event->event_info.got_ip6.if_index == RPC_TCPIP_ADAPTER_IF_ETH)
        {
            setStatusBits(RPC_ETH_CONNECTED_BIT | RPC_ETH_HAS_IP6_BIT);
        }
    }

    for (uint32_t i = 0; i < cbEventList.size(); i++)
    {
        rpcWiFiEventCbList_t entry = cbEventList[i];
        if (entry.cb || entry.fcb || entry.scb)
        {
            if (entry.event == (rpc_system_event_id_t)event->event_id || entry.event == RPC_SYSTEM_EVENT_MAX)
            {
                if (entry.cb)
                {
                    entry.cb((rpc_system_event_id_t)event->event_id);
                }
                else if (entry.fcb)
                {
                    entry.fcb((rpc_system_event_id_t)event->event_id, (rpc_system_event_info_t)event->event_info);
                }
                else
                {
                    entry.scb(event);
                }
            }
        }
    }
    return ESP_OK;
}

/**
 * Return the current channel associated with the network
 * @return channel (1-13)
 */
int32_t rpcWiFiGenericClass::channel(void)
{
    //TO DO
    int primaryChan = 0;
    if (!lowLevelInitDone)
    {
        return primaryChan;
    }
    wifi_get_channel(&primaryChan);
    return primaryChan;
}

/**
 * store rpcWiFi config in SDK flash area
 * @param persistent
 */
void rpcWiFiGenericClass::persistent(bool persistent)
{
    _persistent = persistent;
}

/**
 * set new mode
 * @param m rpcWiFiMode_t
 */
bool rpcWiFiGenericClass::mode(rpc_wifi_mode_t m)
{

    rpc_wifi_mode_t cm = getMode();
    if (cm == m)
    {
        return true;
    }
    if (!cm && m)
    {
        if (!espWiFiStart(_persistent))
        {
            return false;
        }
    }
    else if (cm && !m)
    {
        return espWiFiStop();
    }
    rpc_esp_err_t err;
    wifi_off();
    vTaskDelay(20);
    err = wifi_on(m);
    if (err)
    {
        log_e("Could not set mode! %d", err);
        return false;
    }
    _wifi_mode = m;
    return true;
}

/**
 * get rpcWiFi mode
 * @return WiFiMode
 */
rpc_wifi_mode_t rpcWiFiGenericClass::getMode()
{
    if (!_esp_wifi_started)
    {
        return RPC_WIFI_MODE_NULL;
    }
    return _wifi_mode;
}

/**
 * control STA mode
 * @param enable bool
 * @return ok
 */
bool rpcWiFiGenericClass::enableSTA(bool enable)
{

    rpc_wifi_mode_t currentMode = getMode();
    bool isEnabled = ((currentMode & RPC_WIFI_MODE_STA) != 0);

    if (isEnabled != enable)
    {
        if (enable)
        {
            return mode((rpc_wifi_mode_t)(currentMode | RPC_WIFI_MODE_STA));
        }
        return mode((rpc_wifi_mode_t)(currentMode & (~RPC_WIFI_MODE_STA)));
    }
    return true;
}

/**
 * control AP mode
 * @param enable bool
 * @return ok
 */
bool rpcWiFiGenericClass::enableAP(bool enable)
{

    rpc_wifi_mode_t currentMode = getMode();
    bool isEnabled = ((currentMode & RPC_WIFI_MODE_AP) != 0);

    if (isEnabled != enable)
    {
        if (enable)
        {
            return mode((rpc_wifi_mode_t)(currentMode | RPC_WIFI_MODE_AP));
        }
        return mode((rpc_wifi_mode_t)(currentMode & (~RPC_WIFI_MODE_AP)));
    }
    return true;
}

/**
 * control modem sleep when only in STA mode
 * @param enable bool
 * @return ok
 */
bool rpcWiFiGenericClass::setSleep(bool enable)
{
    if ((getMode() & RPC_WIFI_MODE_STA) == 0)
    {
        log_w("STA has not been started");
        return false;
    }
    return false;
}

/**
 * get modem sleep enabled
 * @return true if modem sleep is enabled
 */
bool rpcWiFiGenericClass::getSleep()
{
    //rpc_wifi_ps_type_t ps;
    if ((getMode() & RPC_WIFI_MODE_STA) == 0)
    {
        log_w("STA has not been started");
        return false;
    }
    return false;
}

/**
 * control rpcWiFi tx power
 * @param power enum maximum rpcWiFi tx power
 * @return ok
 */
bool rpcWiFiGenericClass::setTxPower(rpc_wifi_power_t power)
{
    if ((getStatusBits() & (RPC_STA_STARTED_BIT | RPC_AP_STARTED_BIT)) == 0)
    {
        log_w("Neither AP or STA has been started");
        return false;
    }
    _wifi_power = power;
    return true;
}

rpc_wifi_power_t rpcWiFiGenericClass::getTxPower()
{

    if ((getStatusBits() & (RPC_STA_STARTED_BIT | RPC_AP_STARTED_BIT)) == 0)
    {
        log_w("Neither AP or STA has been started");
        return RPC_WIFI_POWER_19_5dBm;
    }
    return _wifi_power;
}

// -----------------------------------------------------------------------------------------------------------------------
// ------------------------------------------------ Generic Network function ---------------------------------------------
// -----------------------------------------------------------------------------------------------------------------------

/**
 * DNS callback
 * @param name
 * @param ipaddr
 * @param callback_arg
 */
static void wifi_dns_found_callback(const char *name, const new_ip_addr_t *ipaddr, void *callback_arg)
{
    if (ipaddr)
    {
        (*reinterpret_cast<IPAddress *>(callback_arg)) = ipaddr->u_addr.ip4.addr;
    }
    xEventGroupSetBits(_network_event_group, RPC_WIFI_DNS_DONE_BIT);
}

/**
 * Resolve the given hostname to an IP address.
 * @param aHostname     Name to be resolved
 * @param aResult       IPAddress structure to store the returned IP address
 * @return 1 if aIPAddrString was successfully converted to an IP address,
 *          else error code
 */
int rpcWiFiGenericClass::hostByName(const char *aHostname, IPAddress &aResult)
{
    new_ip_addr_t addr;
    aResult = static_cast<uint32_t>(0);
    waitStatusBits(RPC_WIFI_DNS_IDLE_BIT, 5000);
    clearStatusBits(RPC_WIFI_DNS_IDLE_BIT);
    err_t err = new_dns_gethostbyname(aHostname, &addr, &wifi_dns_found_callback, &aResult);
    if(err == ERR_OK && addr.u_addr.ip4.addr) {
        aResult = addr.u_addr.ip4.addr;
    } else if(err == ERR_INPROGRESS) {
        waitStatusBits(RPC_WIFI_DNS_DONE_BIT, 4000);
        clearStatusBits(RPC_WIFI_DNS_DONE_BIT);
    }
    setStatusBits(RPC_WIFI_DNS_IDLE_BIT);
    if((uint32_t)aResult == 0){
        log_e("DNS Failed for %s", aHostname);
    }
    return (uint32_t)aResult != 0;
}

IPAddress rpcWiFiGenericClass::calculateNetworkID(IPAddress ip, IPAddress subnet)
{
    IPAddress networkID;

    for (size_t i = 0; i < 4; i++)
        networkID[i] = subnet[i] & ip[i];

    return networkID;
}

IPAddress rpcWiFiGenericClass::calculateBroadcast(IPAddress ip, IPAddress subnet)
{
    IPAddress broadcastIp;

    for (int i = 0; i < 4; i++)
        broadcastIp[i] = ~subnet[i] | ip[i];

    return broadcastIp;
}

uint8_t rpcWiFiGenericClass::calculateSubnetCIDR(IPAddress subnetMask)
{
    uint8_t CIDR = 0;

    for (uint8_t i = 0; i < 4; i++)
    {
        if (subnetMask[i] == 0x80) // 128
            CIDR += 1;
        else if (subnetMask[i] == 0xC0) // 192
            CIDR += 2;
        else if (subnetMask[i] == 0xE0) // 224
            CIDR += 3;
        else if (subnetMask[i] == 0xF0) // 242
            CIDR += 4;
        else if (subnetMask[i] == 0xF8) // 248
            CIDR += 5;
        else if (subnetMask[i] == 0xFC) // 252
            CIDR += 6;
        else if (subnetMask[i] == 0xFE) // 254
            CIDR += 7;
        else if (subnetMask[i] == 0xFF) // 255
            CIDR += 8;
    }

    return CIDR;
}
