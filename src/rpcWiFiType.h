/*
 ESP8266WiFiType.h - esp8266 Wifi support.
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


#ifndef RPCESP32WIFITYPE_H_
#define RPCESP32WIFITYPE_H_

#define RPC_WIFI_SCAN_RUNNING   (-1)
#define RPC_WIFI_SCAN_FAILED    (-2)

#define rpcWiFiMode_t   rpc_wifi_mode_t
#define rpcWIFI_OFF     RPC_WIFI_MODE_NULL
#define rpcWIFI_STA     RPC_WIFI_MODE_STA
#define rpcWIFI_AP      RPC_WIFI_MODE_AP
#define rpcWIFI_AP_STA  RPC_WIFI_MODE_APSTA

#define rpcWiFiEvent_t  rpc_system_event_id_t
#define rpcWiFiEventInfo_t rpc_system_event_info_t
#define rpcWiFiEventId_t rpc_wifi_event_id_t

typedef enum  {
    RPC_WL_NO_SHIELD        = 255,   // for compatibility with WiFi Shield library
    RPC_WL_IDLE_STATUS      = 0,
    RPC_WL_NO_SSID_AVAIL    = 1,
    RPC_WL_SCAN_COMPLETED   = 2,
    RPC_WL_CONNECTED        = 3,
    RPC_WL_CONNECT_FAILED   = 4,
    RPC_WL_CONNECTION_LOST  = 5,
    RPC_WL_DISCONNECTED     = 6
} rpc_wl_status_t;

#endif /* ESP32WIFITYPE_H_ */
