/**
 *
 * @file rpcWiFiMulti.cpp
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

#include "rpcWiFiMulti.h"
#include <limits.h>
#include <string.h>


rpcWiFiMulti::rpcWiFiMulti()
{
}

rpcWiFiMulti::~rpcWiFiMulti()
{
    for(uint32_t i = 0; i < APlist.size(); i++) {
        rpcWifiAPlist_t entry = APlist[i];
        if(entry.ssid) {
            free(entry.ssid);
        }
        if(entry.passphrase) {
            free(entry.passphrase);
        }
    }
    APlist.clear();
}

bool rpcWiFiMulti::addAP(const char* ssid, const char *passphrase)
{
    rpcWifiAPlist_t newAP;

    if(!ssid || *ssid == 0x00 || strlen(ssid) > 31) {
        // fail SSID too long or missing!
        log_e("[rpcWiFi][APlistAdd] no ssid or ssid too long");
        return false;
    }

    if(passphrase && strlen(passphrase) > 63) {
        // fail passphrase too long!
        log_e("[rpcWiFi][APlistAdd] passphrase too long");
        return false;
    }

    newAP.ssid = strdup(ssid);

    if(!newAP.ssid) {
        log_e("[rpcWiFi][APlistAdd] fail newAP.ssid == 0");
        return false;
    }

    if(passphrase && *passphrase != 0x00) {
        newAP.passphrase = strdup(passphrase);
        if(!newAP.passphrase) {
            log_e("[rpcWiFi][APlistAdd] fail newAP.passphrase == 0");
            free(newAP.ssid);
            return false;
        }
    } else {
        newAP.passphrase = NULL;
    }

    APlist.push_back(newAP);
    log_i("[rpcWiFi][APlistAdd] add SSID: %s", newAP.ssid);
    return true;
}

uint8_t rpcWiFiMulti::run(uint32_t connectTimeout)
{
    int8_t scanResult;
    uint8_t status = rpcWiFi.status();
    if(status == RPC_WL_CONNECTED) {
        for(uint32_t x = 0; x < APlist.size(); x++) {
            if(rpcWiFi.SSID()==APlist[x].ssid) {
                return status;
            }
        }
        rpcWiFi.disconnect(false,false);
        delay(10);
        status = rpcWiFi.status();
    }

    scanResult = rpcWiFi.scanNetworks();
    if(scanResult == RPC_WIFI_SCAN_RUNNING) {
        // scan is running
        return RPC_WL_NO_SSID_AVAIL;
    } else if(scanResult >= 0) {
        // scan done analyze
        rpcWifiAPlist_t bestNetwork { NULL, NULL };
        int bestNetworkDb = INT_MIN;
        uint8_t bestBSSID[6];
        int32_t bestChannel = 0;

        log_i("[rpcWiFi] scan done");

        if(scanResult == 0) {
            log_e("[rpcWiFi] no networks found");
        } else {
            log_i("[rpcWiFi] %d networks found", scanResult);
            for(int8_t i = 0; i < scanResult; ++i) {

                String ssid_scan;
                int32_t rssi_scan;
                uint8_t sec_scan;
                uint8_t* BSSID_scan;
                int32_t chan_scan;

                rpcWiFi.getNetworkInfo(i, ssid_scan, sec_scan, rssi_scan, BSSID_scan, chan_scan);

                bool known = false;
                for(uint32_t x = 0; x < APlist.size(); x++) {
                    rpcWifiAPlist_t entry = APlist[x];

                    if(ssid_scan == entry.ssid) { // SSID match
                        known = true;
                        if(rssi_scan > bestNetworkDb) { // best network
                            if(sec_scan == RPC_WIFI_AUTH_OPEN || entry.passphrase) { // check for passphrase if not open wlan
                                bestNetworkDb = rssi_scan;
                                bestChannel = chan_scan;
                                memcpy((void*) &bestNetwork, (void*) &entry, sizeof(bestNetwork));
                                memcpy((void*) &bestBSSID, (void*) BSSID_scan, sizeof(bestBSSID));
                            }
                        }
                        break;
                    }
                }

                if(known) {
                    log_d(" --->   %d: [%d][%02X:%02X:%02X:%02X:%02X:%02X] %s (%d) %c", i, chan_scan, BSSID_scan[0], BSSID_scan[1], BSSID_scan[2], BSSID_scan[3], BSSID_scan[4], BSSID_scan[5], ssid_scan.c_str(), rssi_scan, (sec_scan == RPC_WIFI_AUTH_OPEN) ? ' ' : '*');
                } else {
                    log_d("       %d: [%d][%02X:%02X:%02X:%02X:%02X:%02X] %s (%d) %c", i, chan_scan, BSSID_scan[0], BSSID_scan[1], BSSID_scan[2], BSSID_scan[3], BSSID_scan[4], BSSID_scan[5], ssid_scan.c_str(), rssi_scan, (sec_scan == RPC_WIFI_AUTH_OPEN) ? ' ' : '*');
                }
            }
        }

        // clean up ram
        rpcWiFi.scanDelete();

        if(bestNetwork.ssid) {
            log_i("[rpcWiFi] Connecting BSSID: %02X:%02X:%02X:%02X:%02X:%02X SSID: %s Channal: %d (%d)", bestBSSID[0], bestBSSID[1], bestBSSID[2], bestBSSID[3], bestBSSID[4], bestBSSID[5], bestNetwork.ssid, bestChannel, bestNetworkDb);

            rpcWiFi.begin(bestNetwork.ssid, bestNetwork.passphrase, bestChannel, bestBSSID);
            status = rpcWiFi.status();

            auto startTime = millis();
            // wait for connection, fail, or timeout
            while(status != RPC_WL_CONNECTED && status != RPC_WL_NO_SSID_AVAIL && status != RPC_WL_CONNECT_FAILED && (millis() - startTime) <= connectTimeout) {
                delay(10);
                status = rpcWiFi.status();
            }

            switch(status) {
            case RPC_WL_CONNECTED:
                log_i("[rpcWiFi] Connecting done.");
                log_d("[rpcWiFi] SSID: %s", rpcWiFi.SSID().c_str());
                log_d("[rpcWiFi] IP: %s", rpcWiFi.localIP().toString().c_str());
                log_d("[rpcWiFi] MAC: %s", rpcWiFi.BSSIDstr().c_str());
                log_d("[rpcWiFi] Channel: %d", rpcWiFi.channel());
                break;
            case RPC_WL_NO_SSID_AVAIL:
                log_e("[rpcWiFi] Connecting Failed AP not found.");
                break;
            case RPC_WL_CONNECT_FAILED:
                log_e("[rpcWiFi] Connecting Failed.");
                break;
            default:
                log_e("[rpcWiFi] Connecting Failed (%d).", status);
                break;
            }
        } else {
            log_e("[rpcWiFi] no matching rpcWiFi found!");
        }
    } else {
        // start scan
        log_d("[rpcWiFi] delete old rpcWiFi config...");
        rpcWiFi.disconnect();

        log_d("[rpcWiFi] start scan");
        // scan rpcWiFi async mode
        rpcWiFi.scanNetworks(true);
    }

    return status;
}
