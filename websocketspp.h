#pragma once
#include <cstring>
#include <cstdint>
#include <vector>
#include <libwebsockets.h>
namespace websocketspp {


  class WebSocketSession {

  };

  class WebSocketProtocol {
  private:

  };


  //! WebSocket base class
  class WebSocket {
  private:
    //! Supported protocols
    std::vector<libwebsocket_protocols>   _protocols;

    //! Context
    libwebsocket_context*       _context;

    //! Context creation info
    lws_context_creation_info   _context_info;


    static int onHTTP(libwebsocket_context* context, libwebsocket* wsi, libwebsocket_callback_reasons reason, void* user_data, void* data, size_t len) {

    }
  public:

    //! Constructor
    WebSocket()
    : _context(nullptr) {
      lws_context_creation_info info;
      info.iface = "0.0.0.0";
libwebsocket_protocols x;


      _context = libwebsocket_create_context(&info);
    }

    //! Destructor
    virtual ~WebSocket() {
      libwebsocket_context_destroy(_context);
    }


  };


  class WebSocketServer : public WebSocket {
  private:


  public:

    //! Constructor
    WebSocketServer() {

    }

    //! Destructor
    ~WebSocketServer() {

    }
  };

  class WebSocketClient : public WebSocket {

  };


}
