/**
 *
 * @file ESP8266WiFiMulti.h
 * @date 16.05.2015
 * @author Markus Sattler
 *
 * Copyright (c) 2015 Markus Sattler. All rights reserved.
 * This file is part of the esp8266 core for Arduino environment.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifndef RPCWIFICLIENTMULTI_H_
#define RPCWIFICLIENTMULTI_H_

#include "rpcWiFi.h"
#include <vector>

typedef struct {
    char * ssid;
    char * passphrase;
} rpcWifiAPlist_t;

class rpcWiFiMulti
{
public:
    rpcWiFiMulti();
    ~rpcWiFiMulti();

    bool addAP(const char* ssid, const char *passphrase = NULL);

    uint8_t run(uint32_t connectTimeout=5000);

private:
    std::vector<rpcWifiAPlist_t> APlist;
};

#endif /* WIFICLIENTMULTI_H_ */
