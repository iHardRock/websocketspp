#pragma once
#include <cstring>
#include <cstdint>
#include <memory>
#include <mutex>
#include <vector>
#include <map>
#include <libwebsockets.h>
#include <uuidpp.h>
namespace websocketspp {
  class WebSocketSession;
  typedef std::shared_ptr<WebSocketSession> PWebSocketSession;


  //! WebSocket session
  class WebSocketSession : public std::enable_shared_from_this<WebSocketSession> {
  public:
    //! Session mode
    enum class Mode {
      Unknown,
      Server,
      Client
    };

  private:
    //! WebSocket mode
    Mode                _mode;

    //! Session object
    lws*                _session;

    //! Session identifier
    UUID                _session_id;


  public:

    //! Default constructor
    WebSocketSession()
    : _mode(Mode::Unknown)
    , _session(nullptr) {

    }

    //! Constructor
    WebSocketSession(lws* session, const UUID& session_id, Mode mode)
    : _mode(mode)
    , _session(session)
    , _session_id(session_id) {

    }

    //! Get WebSocket mode
    Mode getMode() const {
      return _mode;
    }

    //! Get pointer to session
    PWebSocketSession pointer() {
      return shared_from_this();
    }

    //! Get session identifier
    const UUID& getSessionId() const {
      return _session_id;
    }


    //! Send data
    void send(const std::uint8_t* buffer, std::size_t buffer_size) {
      lws_write(_session, const_cast<unsigned char*>(buffer), buffer_size, LWS_WRITE_TEXT);
    }

    //! On connection established
    virtual void onConnected() {

    }

    //! On data received
    virtual void onReceive(const std::uint8_t* buffer, std::size_t buffer_size, std::size_t awaiting_size) {
      printf("> Receive [%ld/%ld]: \n", buffer_size, awaiting_size);
    }

    //! On connection closed
    virtual void onDisconnected() {

    }


  };

  //! WebSocket protocol information
  class WebSocketProtocol {
  private:

    //! Protocol identifier
    std::uint32_t   _protocol_id;

    //! Protocol name
    std::string     _protocol_name;

    //! Receive buffer size
    std::size_t     _rx_buffer_size;

    //! Per-session data size
    std::size_t     _per_session_data_size;

  public:

    //! Constructor
    WebSocketProtocol()
    : _protocol_id(0)
    , _protocol_name("default")
    , _rx_buffer_size(0)
    , _per_session_data_size(0)
    { }

    //! Get protocol name
    const std::string& getName() const {
      return _protocol_name;
    }

    //! Get receive buffer size
    std::size_t getReceiveBufferSize() const {
      return _rx_buffer_size;
    }

    //! Get protocol identifier
    std::uint32_t getProtocolId() const {
      return _protocol_id;
    }

    std::size_t getPerSessionDataSize() const {
      return _per_session_data_size;
    }

    //! Set protocol name
    WebSocketProtocol& setName(const std::string& name) {
      _protocol_name = name;
      return *this;
    }

    //! Set protocol identifier
    WebSocketProtocol& setProtocolId(std::uint32_t id) {
      _protocol_id = id;
      return *this;
    }

    //! Set receive buffer size
    WebSocketProtocol& setReceiveBufferSize(std::size_t buffer_size) {
      _rx_buffer_size = buffer_size;
      return *this;
    }

    //! Set per-session data size
    WebSocketProtocol& setPerSessionDataSize(std::size_t data_size) {
      _per_session_data_size = data_size;
      return *this;
    }
  };



  //! WebSocket base class
  class WebSocket {
  public:

  private:
    //! WebSocket mode
    WebSocketSession::Mode    _mode;

    //! Context
    lws_context*              _context;

  protected:

    //! Access LWS context
    lws_context* getContext() {
      return _context;
    }

    //! Set LWS context
    void setContext(lws_context* context) {
      _context = context;
    }

  public:

    //! Default constructor
    WebSocket()
    : _mode(WebSocketSession::Mode::Unknown)
    , _context(nullptr) {

    }

    //! Constructor
    WebSocket(WebSocketSession::Mode mode)
    : _mode(mode)
    , _context(nullptr) {

    }

    //! Destructor
    virtual ~WebSocket() {
      // - Destroy context
      if (_context) {
        lws_context_destroy(_context);
        _context = nullptr;
      }
    }

    //! Get WebSocket mode
    WebSocketSession::Mode getMode() const {
      return _mode;
    }

    //! On WebSocket event triggered
    int __onEvent__(lws_callback_reasons reason, void* arg, void* payload, std::size_t payload_len) {
      switch (reason) {
        default:
          break;
      }
      return 0;
    }


  };



  //! WebSocket server
  class WebSocketServer : public WebSocket {
  private:
    //! Context creation info
    lws_context_creation_info                 _context_options;

    //! Supported protocols
    std::map<std::string, WebSocketProtocol>  _protocols;

    //! Supported protocols (plain data for libwebsockets)
    std::vector<lws_protocols>                _protocols_plain;

    //! Bind interface
    std::string                               _listen_interface;

    //! Server VHost name
    std::string                               _vhost_name;

    //! Active sessions by session identifier
    std::map<UUID, PWebSocketSession>         _sessions_by_id;

    //! Active sessions by wsi
    std::map<lws*, PWebSocketSession>         _sessions_by_wsi;

    //! Active sessions mutex
    std::mutex                                _sessions_mutex;

  private:

    //! Access LWS context information
    lws_context_creation_info* getContextOptions() {
      return &_context_options;
    }

    //! Get plain protocols collection
    const lws_protocols* getProtocolsPlain() const {
      return _protocols_plain.data();
    }


    //! Create new session
    void createSession(lws* wsi) {
      // - Create session object
      PWebSocketSession session = std::make_shared<WebSocketSession>(wsi, UUID::generate(), getMode());
      { std::lock_guard<std::mutex> lock(_sessions_mutex);
        // - Register session
       _sessions_by_id[session->getSessionId()] = session;
       _sessions_by_wsi[wsi] = session;
      }
    }


    //! Unregister session from server
    void destorySession(lws* wsi) {
      std::lock_guard<std::mutex> lock(_sessions_mutex);

      // - Find session
      auto i_session = _sessions_by_wsi.find(wsi);
      if (i_session == _sessions_by_wsi.end()) return;

      // - Remove session
      _sessions_by_id.erase(i_session->second->getSessionId());
      _sessions_by_wsi.erase(wsi);
    }

    //! Find session by WSI
    PWebSocketSession findSessionByWSI(lws* wsi) {
      std::lock_guard<std::mutex> lock(_sessions_mutex);

      // - Find session
      auto i_session = _sessions_by_wsi.find(wsi);
      if (i_session == _sessions_by_wsi.end()) return nullptr;
      else return i_session->second;
    }


    //! WebSocket callback
    static int __callback_function__(lws* wsi, lws_callback_reasons reason, void* user_data, void* payload, std::size_t payload_size) {
      // - Access context
      lws_context* context = lws_get_context(wsi);

      // - Access associated server
      WebSocketServer* server = reinterpret_cast<WebSocketServer*>(lws_context_user(context));

      // - Handle callback reason
      switch (reason) {
        case LWS_CALLBACK_ADD_POLL_FD:
        case LWS_CALLBACK_DEL_POLL_FD:
        case LWS_CALLBACK_LOCK_POLL:
        case LWS_CALLBACK_UNLOCK_POLL:
        case LWS_CALLBACK_CHANGE_MODE_POLL_FD:
        case LWS_CALLBACK_GET_THREAD_ID:
          return 0;

        // ------------------------------------------------------------------------------------------------------------
        // --- Create new session
        // ------------------------------------------------------------------------------------------------------------
        case LWS_CALLBACK_WSI_CREATE:
          server->createSession(wsi);
          break;



        case LWS_CALLBACK_PROTOCOL_INIT:
          printf("> Protocol Init\n");
          return 0;

        case LWS_CALLBACK_FILTER_NETWORK_CONNECTION:
          printf("> Filter Network\n");
          return 0;


        case LWS_CALLBACK_SERVER_NEW_CLIENT_INSTANTIATED:
          printf("> New client instance\n");
          return 0;

        case LWS_CALLBACK_FILTER_PROTOCOL_CONNECTION:
          printf("> Filter protocol\n");
          return 0;


        // ------------------------------------------------------------------------------------------------------------
        // --- Connection established
        // ------------------------------------------------------------------------------------------------------------
        case LWS_CALLBACK_ESTABLISHED: {
          PWebSocketSession session = server->findSessionByWSI(wsi);
          session->onConnected();
        }; break;


        // ------------------------------------------------------------------------------------------------------------
        // --- Data received
        // ------------------------------------------------------------------------------------------------------------
        case LWS_CALLBACK_RECEIVE: {
          PWebSocketSession session = server->findSessionByWSI(wsi);
          session->onReceive(reinterpret_cast<const std::uint8_t*>(payload), payload_size, lws_remaining_packet_payload(wsi));
        }; break;


        case LWS_CALLBACK_WS_PEER_INITIATED_CLOSE:
          printf("> Close request\n");
          break;


        // ------------------------------------------------------------------------------------------------------------
        // --- Connection closed
        // ------------------------------------------------------------------------------------------------------------
        case LWS_CALLBACK_CLOSED: {
          PWebSocketSession session = server->findSessionByWSI(wsi);
          session->onDisconnected();
        }; break;

        // ------------------------------------------------------------------------------------------------------------
        // --- Destroy session
        // ------------------------------------------------------------------------------------------------------------
        case LWS_CALLBACK_WSI_DESTROY:
          server->destorySession(wsi);
          break;


        default:
          printf("*** %ld\n", (std::size_t)reason);
          break;
      }

      return 0;


      // - Forwad call into WebSocket object
      //return websocket_ptr->__onEvent__(reason, user_data, payload, payload_len);
    }

  public:

    //! Constructor
    WebSocketServer()
    : WebSocket(WebSocketSession::Mode::Server) {
      // - Release context info
      memset(&_context_options, 0x00, sizeof(_context_options));

      // - Set generic context options
      getContextOptions()->user = this;
    }

    //! Destructor
    ~WebSocketServer() {
    }

    //! Add new protocol
    bool addProtocol(const WebSocketProtocol& protocol) {
      // - Check is protocol already exists
      if (_protocols.find(protocol.getName()) != _protocols.end()) return false;
      _protocols[protocol.getName()] = protocol;
      return true;
    }

    //! Remove protocol
    bool removeProtocol(const std::string& protocol_name) {
      auto i_protocol = _protocols.find(protocol_name);
      if (i_protocol == _protocols.end()) return false;
      _protocols.erase(i_protocol);
      return true;
    }

    //! Set listen interface
    void setListenInterface(const std::string& interface_name) {
      _listen_interface = interface_name;
      getContextOptions()->iface = _listen_interface.empty() ? nullptr : _listen_interface.c_str();
    }

    //! Set listen interface
    void setListenPort(std::uint16_t port) {
      getContextOptions()->port = port;
    }

    //! Set keep-alive timeout
    void setKeepAliveTimeout(std::uint32_t timeout_sec) {
      getContextOptions()->ka_time = timeout_sec;
    }

    //! Set keep-alive retiry count before drop connection
    void setKeepAliveRetryCount(std::uint32_t count) {
      getContextOptions()->ka_probes = count;
    }

    //! Set timeout before next keep-alive retry
    void setKeepAliveRetryTimeout(std::uint32_t timeout_sec) {
      getContextOptions()->ka_time = timeout_sec;
    }

    //! Set server virtual host name
    void setHostName(const std::string& name) {
      _vhost_name = name;
      getContextOptions()->vhost_name = _vhost_name.empty() ? nullptr : _vhost_name.c_str();
    }

    //! Set ping interval
    void setPingInterval(std::uint16_t interval_sec) {
      getContextOptions()->ws_ping_pong_interval = interval_sec;
    }

    //! Set ping timeout before connection drop
    void setPingTimeout(std::uint16_t timeout_sec) {
      getContextOptions()->timeout_secs = timeout_sec;
    }


    //! Start WebSocket
    bool start() {
      // - Check is protocols set
      if (_protocols.empty()) return false;

      // - Build protocols collection
      _protocols_plain.clear();
      for (auto& protocol : _protocols) {
        // - Create protocol descriptor
        lws_protocols protocol_descriptor;
        memset(&protocol_descriptor, 0x00, sizeof(protocol_descriptor));
        protocol_descriptor.id                    = protocol.second.getProtocolId();
        protocol_descriptor.name                  = protocol.second.getName().c_str();
        protocol_descriptor.rx_buffer_size        = protocol.second.getReceiveBufferSize();
        protocol_descriptor.per_session_data_size = protocol.second.getPerSessionDataSize();
        protocol_descriptor.user                  = nullptr;
        protocol_descriptor.callback              = &WebSocketServer::__callback_function__;

        // - Register protocol descriptor
        _protocols_plain.push_back(protocol_descriptor);
      }
      // - Append final empty descriptor
      lws_protocols last_protocol_descriptor;
      memset(&last_protocol_descriptor, 0x00, sizeof(last_protocol_descriptor));
      _protocols_plain.push_back(last_protocol_descriptor);

      // - Set context options
      getContextOptions()->protocols = getProtocolsPlain();

      // - Create context
      setContext(lws_create_context(getContextOptions()));

      // - Check is context created
      if (!getContext()) return false;
      return true;
    }


    //! Update server
    void update() {
      lws_service(getContext(), 50);
    }
  };



  class WebSocketClient : public WebSocket {
    WebSocketClient() {


    }
  };


}
