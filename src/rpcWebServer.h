/*
  rpcWebServer.h - Dead simple web-server.
  Supports only one simultaneous client, knows how to handle GET and POST.

  Copyright (c) 2014 Ivan Grokhotkov. All rights reserved.

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
  Modified 8 May 2015 by Hristo Gochkov (proper post and file upload handling)
*/


#ifndef RPCWEBSERVER_H
#define RPCWEBSERVER_H

#include <functional>
#include <memory>
#include <rpcWiFi.h>
#include "HTTP_Method.h"

enum rpcHTTPUploadStatus { RPC_UPLOAD_FILE_START, RPC_UPLOAD_FILE_WRITE, RPC_UPLOAD_FILE_END,
                        RPC_UPLOAD_FILE_ABORTED };
enum rpcHTTPClientStatus { RPC_HC_NONE, RPC_HC_WAIT_READ, RPC_HC_WAIT_CLOSE };
enum rpcHTTPAuthMethod { RPC_BASIC_AUTH, RPC_DIGEST_AUTH };

#define HTTP_DOWNLOAD_UNIT_SIZE 1436

#ifndef HTTP_UPLOAD_BUFLEN
#define HTTP_UPLOAD_BUFLEN 1436
#endif

#define HTTP_MAX_DATA_WAIT 5000 //ms to wait for the client to send the request
#define HTTP_MAX_POST_WAIT 5000 //ms to wait for POST data to arrive
#define HTTP_MAX_SEND_WAIT 5000 //ms to wait for data chunk to be ACKed
#define HTTP_MAX_CLOSE_WAIT 2000 //ms to wait for the client to close the connection

#define CONTENT_LENGTH_UNKNOWN ((size_t) -1)
#define CONTENT_LENGTH_NOT_SET ((size_t) -2)

class rpcWebServer;

typedef struct {
  rpcHTTPUploadStatus status;
  String  filename;
  String  name;
  String  type;
  size_t  totalSize;    // file size
  size_t  currentSize;  // size of data currently in buf
  uint8_t buf[HTTP_UPLOAD_BUFLEN];
} rpcHTTPUpload;

#include "detail/RequestHandler.h"

namespace fs {
class FS;
}

class rpcWebServer
{
public:
  rpcWebServer(IPAddress addr, int port = 80);
  rpcWebServer(int port = 80);
  virtual ~rpcWebServer();

  virtual void begin();
  virtual void begin(uint16_t port);
  virtual void handleClient();

  virtual void close();
  void stop();

  bool authenticate(const char * username, const char * password);
  void requestAuthentication(rpcHTTPAuthMethod mode = RPC_BASIC_AUTH, const char* realm = NULL, const String& authFailMsg = String("") );

  typedef std::function<void(void)> THandlerFunction;
  void on(const String &uri, THandlerFunction handler);
  void on(const String &uri, HTTPMethod method, THandlerFunction fn);
  void on(const String &uri, HTTPMethod method, THandlerFunction fn, THandlerFunction ufn);
  void addHandler(RequestHandler* handler);
  void serveStatic(const char* uri, fs::FS& fs, const char* path, const char* cache_header = NULL );
  void onNotFound(THandlerFunction fn);  //called when handler is not assigned
  void onFileUpload(THandlerFunction fn); //handle file uploads

  String uri() { return _currentUri; }
  HTTPMethod method() { return _currentMethod; }
  virtual rpcWiFiClient client() { return _currentClient; }
  rpcHTTPUpload& upload() { return *_currentUpload; }

  String pathArg(unsigned int i); // get request path argument by number
  String arg(String name);        // get request argument value by name
  String arg(int i);              // get request argument value by number
  String argName(int i);          // get request argument name by number
  int args();                     // get arguments count
  bool hasArg(String name);       // check if argument exists
  void collectHeaders(const char* headerKeys[], const size_t headerKeysCount); // set the request headers to collect
  String header(String name);      // get request header value by name
  String header(int i);              // get request header value by number
  String headerName(int i);          // get request header name by number
  int headers();                     // get header count
  bool hasHeader(String name);       // check if header exists

  String hostHeader();            // get request host header if available or empty String if not

  // send response to the client
  // code - HTTP response code, can be 200 or 404
  // content_type - HTTP content type, like "text/plain" or "image/png"
  // content - actual content body
  void send(int code, const char* content_type = NULL, const String& content = String(""));
  void send(int code, char* content_type, const String& content);
  void send(int code, const String& content_type, const String& content);
  void send_P(int code, PGM_P content_type, PGM_P content);
  void send_P(int code, PGM_P content_type, PGM_P content, size_t contentLength);

  void enableCORS(boolean value = true);
  void enableCrossOrigin(boolean value = true);

  void setContentLength(const size_t contentLength);
  void sendHeader(const String& name, const String& value, bool first = false);
  void sendContent(const String& content);
  void sendContent_P(PGM_P content);
  void sendContent_P(PGM_P content, size_t size);

  static String urlDecode(const String& text);

  template<typename T>
  size_t streamFile(T &file, const String& contentType) {
    _streamFileCore(file.size(), file.name(), contentType);
    uint8_t buffer[512];
    uint32_t remain = file.size();
    uint32_t size = file.size();
    uint16_t bytesToWrite = 0;
    uint32_t count = 0;

    while(true){
    bytesToWrite = remain > 512 ? 512 : remain;
    remain -= bytesToWrite;
    file.read(buffer, bytesToWrite);
    count += _currentClient.write(buffer, bytesToWrite);
    if(remain == 0)
      break;
    }
    return count;
  }

protected:
  virtual size_t _currentClientWrite(const char* b, size_t l) { return _currentClient.write( b, l ); }
  virtual size_t _currentClientWrite_P(PGM_P b, size_t l) { return _currentClient.write_P( b, l ); }
  void _addRequestHandler(RequestHandler* handler);
  void _handleRequest();
  void _finalizeResponse();
  bool _parseRequest(rpcWiFiClient& client);
  void _parseArguments(String data);
  static String _responseCodeToString(int code);
  bool _parseForm(rpcWiFiClient& client, String boundary, uint32_t len);
  bool _parseFormUploadAborted();
  void _uploadWriteByte(uint8_t b);
  int _uploadReadByte(rpcWiFiClient& client);
  void _prepareHeader(String& response, int code, const char* content_type, size_t contentLength);
  bool _collectHeader(const char* headerName, const char* headerValue);

  void _streamFileCore(const size_t fileSize, const String & fileName, const String & contentType);

  String _getRandomHexString();
  // for extracting Auth parameters
  String _extractParam(String& authReq,const String& param,const char delimit = '"');

  struct RequestArgument {
    String key;
    String value;
  };

  boolean     _corsEnabled;
  rpcWiFiServer  _server;

  rpcWiFiClient  _currentClient;
  HTTPMethod  _currentMethod;
  String      _currentUri;
  uint8_t     _currentVersion;
  rpcHTTPClientStatus _currentStatus;
  unsigned long _statusChange;

  RequestHandler*  _currentHandler;
  RequestHandler*  _firstHandler;
  RequestHandler*  _lastHandler;
  THandlerFunction _notFoundHandler;
  THandlerFunction _fileUploadHandler;

  int              _currentArgCount;
  RequestArgument* _currentArgs;
  int              _postArgsLen;
  RequestArgument* _postArgs;

  std::unique_ptr<rpcHTTPUpload> _currentUpload;

  int              _headerKeysCount;
  RequestArgument* _currentHeaders;
  size_t           _contentLength;
  String           _responseHeaders;

  String           _hostHeader;
  bool             _chunked;

  String           _snonce;  // Store noance and opaque for future comparison
  String           _sopaque;
  String           _srealm;  // Store the Auth realm between Calls

};


#endif //ESP8266WEBSERVER_H
