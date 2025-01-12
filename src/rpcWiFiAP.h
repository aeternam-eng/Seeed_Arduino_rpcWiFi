/*
 ESP8266WiFiAP.h - esp8266 Wifi support.
 Based on WiFi.h from Arduino WiFi shield library.
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

#ifndef RPCESP32WIFIAP_H_
#define RPCESP32WIFIAP_H_


#include "rpcWiFiType.h"
#include "rpcWiFiGeneric.h"


class rpcWiFiAPClass
{

    // ----------------------------------------------------------------------------------------------
    // ----------------------------------------- AP function ----------------------------------------
    // ----------------------------------------------------------------------------------------------

public:

    bool softAP(const char* ssid, const char* passphrase, int channel = 11, int ssid_hidden = 0, int max_connection = 4);
    bool softAP(const char *ssid, int channel = 11, int ssid_hidden = 0, int max_connection = 4);
    bool softAPConfig(IPAddress local_ip, IPAddress gateway, IPAddress subnet);
    bool softAPdisconnect(bool wifioff = false);

    uint8_t softAPgetStationNum();

    IPAddress softAPIP();

    IPAddress softAPBroadcastIP();
    IPAddress softAPNetworkID();
    uint8_t softAPSubnetCIDR();

    bool softAPenableIpV6();
    IPv6Address softAPIPv6();

    const char * softAPgetHostname();
    bool softAPsetHostname(const char * hostname);

    uint8_t* softAPmacAddress(uint8_t* mac);
    String softAPmacAddress(void);

protected:
private:
    IPAddress _local_ip = IPAddress(192, 168, 1, 1);
    IPAddress _gateway = IPAddress(192, 168, 1, 1);
    IPAddress _subnet = IPAddress(255, 255, 255, 0);

};

#endif /* ESP32WIFIAP_H_*/
