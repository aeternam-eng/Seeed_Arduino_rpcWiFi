/*
  Client.h - Base class that provides Client
  Copyright (c) 2011 Adrian McEwen.  All right reserved.

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

#ifndef _RPCWIFICLIENT_H_
#define _RPCWIFICLIENT_H_


#include "Arduino.h"
#include "Client.h"
#undef max
#undef min
#include <memory>

class rpcWiFiClientSocketHandle;
class rpcWiFiClientRxBuffer;

class rpcESPLwIPClient : public Client
{
public:
        virtual int connect(IPAddress ip, uint16_t port, int32_t timeout) = 0;
        virtual int connect(const char *host, uint16_t port, int32_t timeout) = 0;
        virtual int setTimeout(uint32_t seconds) = 0;
};

class rpcWiFiClient : public rpcESPLwIPClient
{
protected:
    std::shared_ptr<rpcWiFiClientSocketHandle> clientSocketHandle;
    std::shared_ptr<rpcWiFiClientRxBuffer> _rxBuffer;
    int _localPort;
    bool _connected;

public:
    rpcWiFiClient *next;
    rpcWiFiClient();
    rpcWiFiClient(int fd);
    rpcWiFiClient(uint64_t localport);
    ~rpcWiFiClient();
    int connect(IPAddress ip, uint16_t port);
    int connect(IPAddress ip, uint16_t port, int32_t timeout);
    int connect(const char *host, uint16_t port);
    int connect(const char *host, uint16_t port, int32_t timeout);
    size_t write(uint8_t data);
    size_t write(const uint8_t *buf, size_t size);
    size_t write_P(PGM_P buf, size_t size);
    size_t write(Stream &stream);
    int available();
    int read();
    int read(uint8_t *buf, size_t size);
    int peek();
    void flush();
    void stop();
    uint8_t connected();

    operator bool()
    {
        return connected();
    }
    rpcWiFiClient & operator=(const rpcWiFiClient &other);
    bool operator==(const bool value)
    {
        return bool() == value;
    }
    bool operator!=(const bool value)
    {
        return bool() != value;
    }
    bool operator==(const rpcWiFiClient&);
    bool operator!=(const rpcWiFiClient& rhs)
    {
        return !this->operator==(rhs);
    };

    int fd() const;

    int setSocketOption(int option, char* value, size_t len);
    int setOption(int option, int *value);
    int getOption(int option, int *value);
    int setTimeout(uint32_t seconds);
    int setNoDelay(bool nodelay);
    bool getNoDelay();

    IPAddress remoteIP() const;
    IPAddress remoteIP(int fd) const;
    uint16_t remotePort() const;
    uint16_t remotePort(int fd) const;
    IPAddress localIP() const;
    IPAddress localIP(int fd) const;
    uint16_t localPort() const;
    uint16_t localPort(int fd) const;

    //friend class rpcWiFiServer;
    using Print::write;
    uint32_t conn_staus; // Check the connection status every 3S 
};

#endif /* _WIFICLIENT_H_ */
