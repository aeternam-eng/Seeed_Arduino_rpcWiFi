/*
 WiFi.h - esp32 Wifi support.
 Based on WiFi.h from Arduino WiFi shield library.
 Copyright (c) 2011-2014 Arduino.  All right reserved.
 Modified by Ivan Grokhotkov, December 2014

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

#ifndef RPCWiFi_h
#define RPCWiFi_h

#include <stdint.h>

#include "Print.h"
#include "IPAddress.h"
#include "IPv6Address.h"

#include "WiFiType.h"
#include "rpcWiFiSTA.h"
#include "rpcWiFiAP.h"
#include "rpcWiFiScan.h"
#include "rpcWiFiGeneric.h"

#include "rpcWiFiClient.h"
#include "rpcWiFiServer.h"
#include "rpcWiFiUdp.h"

class rpcWiFiClass : public rpcWiFiGenericClass, public rpcWiFiSTAClass, public rpcWiFiScanClass, public rpcWiFiAPClass
{
public:
    using rpcWiFiGenericClass::channel;

    using rpcWiFiSTAClass::SSID;
    using rpcWiFiSTAClass::RSSI;
    using rpcWiFiSTAClass::BSSID;
    using rpcWiFiSTAClass::BSSIDstr;

    using rpcWiFiScanClass::SSID;
    using rpcWiFiScanClass::encryptionType;
    using rpcWiFiScanClass::RSSI;
    using rpcWiFiScanClass::BSSID;
    using rpcWiFiScanClass::BSSIDstr;
    using rpcWiFiScanClass::channel;

public:
    void printDiag(Print& dest);
    friend class rpcWiFiClient;
    friend class rpcWiFiServer;
    friend class rpcWiFiUDP;
};

extern rpcWiFiClass rpcWiFi;

#endif
