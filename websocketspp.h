#pragma once
#include <arpa/inet.h>
#include <cstring>
#include <cstdint>
#include <memory>
#include <mutex>
#include <vector>
#include <map>
#include <libwebsockets.h>
#include <uuidpp.h>

namespace websocketspp {
  #define WEBSOCKETSPP_DEFAULT_TX_BUFFER_SIZE 65536
  #define WEBSOCKETSPP_DEFAULT_RX_BUFFER_SIZE 65536
  //! WebSocket Protocol
  class WebSocketProtocol;

  //! WebSocket instance
  class WebSocket;

  //! Pointer to WebSocket instance
  typedef std::shared_ptr<WebSocket> PWebSocket;

  //! WebSocket session
  class WebSocketSession;

  //! Pointer to WebSocket session
  typedef std::shared_ptr<WebSocketSession> PWebSocketSession;

  //! WebSocket server
  template < class SessionType > class WebSocketServer;

  //! WebSocket client
  class WebSocketClient;


  //! WebSocket protocol information
  class WebSocketProtocol {
  private:

    //! Protocol identifier
    std::uint32_t   _protocol_id;

    //! Protocol name
    std::string     _protocol_name;

    //! Receive buffer size
    std::size_t     _rx_buffer_size;

    //! Transmit buffer size
    std::size_t     _tx_buffer_size;

    //! Per-session data size
    std::size_t     _per_session_data_size;

  public:

    //! Constructor
    WebSocketProtocol()
    : _protocol_id(0)
    , _protocol_name("default")
    , _rx_buffer_size(WEBSOCKETSPP_DEFAULT_RX_BUFFER_SIZE)
    , _tx_buffer_size(WEBSOCKETSPP_DEFAULT_TX_BUFFER_SIZE)
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

    //! Get transmit buffer size
    std::size_t getTransmitBufferSize() const {
      return _tx_buffer_size;
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

    //! Set transmit buffer size
    WebSocketProtocol& setTransmitBufferSize(std::size_t buffer_size) {
      _tx_buffer_size = buffer_size;
      return *this;
    }

    //! Set per-session data size
    WebSocketProtocol& setPerSessionDataSize(std::size_t data_size) {
      _per_session_data_size = data_size;
      return *this;
    }
  };


  //! WebSocket base class
  class WebSocket : public std::enable_shared_from_this<WebSocket> {
  public:
    //! WebSocket mode
    enum class Mode {
      //! Unknown mode
      Unknown,
      //! Server-side WebSocket
      Server,
      //! Client-side WebSocket
      Client
    };

    //! WebSocket running state
    enum class RunningState {
      Stopped,
      Starting,
      Running,
      Stopping,
    };

  private:
    //! WebSocket mode
    Mode                _mode;

    //! Context
    lws_context*        _context;

    //! WebSocket state
    RunningState        _state;

  protected:

    //! Access LWS context
    lws_context* getContext() const {
      return _context;
    }

    //! Set LWS context
    void setContext(lws_context* context) {
      _context = context;
    }

    //! Destroy context
    void destoryContext() {
      // - Destroy context
      if (_context) {
        lws_context_destroy(_context);
        _context = nullptr;
      }
    }

    //! Set running state
    void setRunningState(RunningState state) {
      _state = state;
    }


  public:

    //! Default constructor
    WebSocket()
    : _mode(Mode::Unknown)
    , _context(nullptr)
    , _state(RunningState::Stopped)
    {}

    //! Constructor
    WebSocket(Mode mode)
    : _mode(mode)
    , _context(nullptr)
    , _state(RunningState::Stopped)
    {}

    //! Destructor
    virtual ~WebSocket() {
      destoryContext();
    }

    //! Get pointer to socket
    PWebSocket pointer() {
      return shared_from_this();
    }

    //! Get pointer to socket of specified type
    template < typename SocketType >
    std::shared_ptr<SocketType> pointer() {
      return std::dynamic_pointer_cast<SocketType>(pointer());
    }

    //! Get WebSocket mode
    Mode getMode() const {
      return _mode;
    }



    //! Get running state
    RunningState getRunningState() const {
      return _state;
    }

    //! Get default TX buffer size
    static constexpr std::size_t getDefaultTXBufferSize() {
      return WEBSOCKETSPP_DEFAULT_TX_BUFFER_SIZE;
    }

    //! Get default RX buffer size
    static constexpr std::size_t getDefaultRXBufferSize() {
      return WEBSOCKETSPP_DEFAULT_RX_BUFFER_SIZE;
    }

  public:
    //! Start WebSocket
    virtual bool start() = 0;

    //! Update WebSocket
    virtual void update() = 0;

    //! Stop WebSocket
    virtual bool stop()  = 0;

    //! On new session connected
    virtual void onConnected(PWebSocketSession session)
    {}

    //! On session disconnected
    virtual void onDisconnected(PWebSocketSession session)
    {}

    //! On data received
    virtual void onReceive(PWebSocketSession session, const std::uint8_t* buffer, std::size_t buffer_size, std::size_t awaiting_size, bool is_final_fragment)
    {}
  };

  //! WebSocket session
  class WebSocketSession : public std::enable_shared_from_this<WebSocketSession> {
  template <class SessionType> friend class WebSocketServer;
  private:
    //! WebSocket mode
    WebSocket::Mode           _mode;

    //! Session object
    lws*                      _session;

    //! Session identifier
    UUID                      _session_id;

    //! Associated websocket
    PWebSocket                _socket;

    //! TX Buffer
    std::vector<std::uint8_t> _tx_buffer;

    //! TX Mutex
    std::recursive_mutex      _tx_mutex;

    //! Shutdown flag
    bool                      _is_shutting_down;

    //! Connection protocol
    WebSocketProtocol*        _protocol;
  private:

    //! Initialize session at creation
    void __init__(lws* session, const UUID& session_id, WebSocket::Mode mode, PWebSocket socket_ptr) {
      _mode       = mode;
      _session    = session;
      _session_id = session_id;
      _socket     = socket_ptr;

    }

    //! Shutdown session
    void __shutdown__() {
      _is_shutting_down = true;
    }

    //! On server become writtable
    void __onWrittable__() {

    }

    //! Set current protocol
    void __setProtocol__(WebSocketProtocol* protocol) {
      _protocol = protocol;

      // - Prepare TX buffer
      _tx_buffer.resize(LWS_PRE + protocol->getTransmitBufferSize());
    }
  public:

    //! Constructor
    WebSocketSession()
    : _mode(WebSocket::Mode::Unknown)
    , _session(nullptr)
    , _is_shutting_down(false)
    , _protocol(nullptr)
    {}

    //! Get WebSocket mode
    WebSocket::Mode getMode() const {
      return _mode;
    }

    //! Access current protocol
    WebSocketProtocol* getProtocol() const {
      return _protocol;
    }

    //! Get pointer to session
    PWebSocketSession pointer() {
      return shared_from_this();
    }

    template < typename SessionType >
    std::shared_ptr<SessionType> pointer() {
      return std::dynamic_pointer_cast<SessionType>(pointer());
    }

    //! Get pointer to socket
    PWebSocket getWebSocket() {
      return _socket;
    }

    //! Get session identifier
    const UUID& getSessionId() const {
      return _session_id;
    }

    //! Send data
    bool send(const std::uint8_t* buffer, std::size_t buffer_size, bool binary_mode = false) {
      // - Check is shutting down
      if (_is_shutting_down) return false;

      // - Send data procedure
      try {
        std::lock_guard<std::recursive_mutex> lock(_tx_mutex);

        // - Bytes sent counter
        std::size_t bytes_sent = 0;

        // - Is message chunked
        bool chunked = false;

        // - Send splitted to chunks by TX buffer size
        while (buffer_size) {
          // - Check is shutting down
          if (_is_shutting_down) return false;

          // - Calculate portion size
          std::size_t portion_size = std::min(buffer_size, _tx_buffer.size() - LWS_PRE);

          // - Copy next portion into buffer
          memcpy(&_tx_buffer[LWS_PRE], &buffer[bytes_sent], portion_size);

          // - Check is need more chunks
          bool more_chunks = (buffer_size - portion_size);

          // - Setup flags
          lws_write_protocol flags = binary_mode ? LWS_WRITE_BINARY : LWS_WRITE_TEXT;

          // - Send continuation frame if chunked
          if (chunked) flags = more_chunks
            ? static_cast<lws_write_protocol>(LWS_WRITE_CONTINUATION | LWS_WRITE_NO_FIN)  // - Need more chunks
            : static_cast<lws_write_protocol>(LWS_WRITE_CONTINUATION)                     // - Last chunk
          ;

          // - Detect first chunk
          if (more_chunks && !chunked) {
            flags   =  (lws_write_protocol)((int)flags | LWS_WRITE_NO_FIN);               // - First chunk
            chunked = true;
          }

          // - Send data
          int tx_size = lws_write(_session, &_tx_buffer[LWS_PRE], portion_size, flags);
          if (tx_size== -1) {
            // - TODO: Close connection
            __shutdown__();
            return false;
          }

          // - Wait for socket become writtable
          while (lws_send_pipe_choked(_session) || lws_partial_buffered(_session)) usleep(1);

          // - Go to next chunk
          bytes_sent  += tx_size;
          buffer_size -= tx_size;
        }
        return true;
      } catch (const std::exception& e) {
        // - Close connection
        __shutdown__();
        return false;
      }
    }

  public:

    //! On connection established
    virtual void onConnected()
    {}

    //! On data received
    virtual void onReceive(const std::uint8_t* buffer, std::size_t buffer_size, std::size_t awaiting_size, bool is_final_fragment)
    {}

    //! On connection closed
    virtual void onDisconnected()
    {}

    //! On disconnect packet received
    virtual bool onDisconnectRequested() {
      return true;
    }
  };


  //! WebSocket server
  template < class SessionType >
  class WebSocketServer : public WebSocket {
  public:
    //! Pointer to WebSocket
    typedef std::shared_ptr<WebSocketServer<SessionType>> ptr;

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

    //! Update timeout
    std::uint32_t                             _update_timeout;

    #ifdef __WITH_ACTIVITY__
    //! Server activity
    activity<WebSocketServer<SessionType>>    _server_activity;
    #endif

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
      PWebSocketSession session = std::make_shared<SessionType>();
      session->__init__(wsi, UUID::generate(), getMode(), pointer());

      // - Register session
      { std::lock_guard<std::mutex> lock(_sessions_mutex);
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

        case LWS_CALLBACK_PROTOCOL_DESTROY:
          printf("> Protocol Destroy\n");
          return 0;

        // ------------------------------------------------------------------------------------------------------------
        // --- Filter network connection
        // ------------------------------------------------------------------------------------------------------------
        case LWS_CALLBACK_FILTER_NETWORK_CONNECTION: {
          // - Get remote address
          sockaddr_storage  remote_addr;
          socklen_t         remote_size   = sizeof(remote_addr);
          int*              socket_handle = reinterpret_cast<int*>(&payload);
          if (getpeername(*socket_handle, reinterpret_cast<sockaddr*>(&remote_addr), &remote_size)) {
            // - Drop connection if unable to get remote address
            return 1;
          }

          // - Select address type
          switch (remote_addr.ss_family) {
            case AF_INET: {
              //sockaddr_in* ipv4 = reinterpret_cast<sockaddr_in*>(&remote_addr);
              //char buffer[INET_ADDRSTRLEN];
              //inet_ntop(AF_INET, &(remote_addr.sin_addr), buffer, INET_ADDRSTRLEN);


            }; break;
            case AF_INET6: {
              //sockaddr_in6* ipv6 = reinterpret_cast<sockaddr_in6*>(&remote_addr);

            }; break;

            // - Drop connection on unknown socket addrress family
            default: return 1;
          }

          // - Accept connection
          return 0;
        }; break;


        // ------------------------------------------------------------------------------------------------------------
        // --- Client instance created
        // ------------------------------------------------------------------------------------------------------------
        case LWS_CALLBACK_SERVER_NEW_CLIENT_INSTANTIATED:
          //printf("> New client instance\n");
          return 0;

        // ------------------------------------------------------------------------------------------------------------
        // --- Setup connection protocol
        // ------------------------------------------------------------------------------------------------------------
        case LWS_CALLBACK_FILTER_PROTOCOL_CONNECTION: {
          WebSocketProtocol*  protocol  = server->getProtocol(user_data ? reinterpret_cast<const char*>(user_data) : "default");
          PWebSocketSession   session   = server->findSessionByWSI(wsi);
          session->__setProtocol__(protocol);
        }; return 0;


        // ------------------------------------------------------------------------------------------------------------
        // --- Connection established
        // ------------------------------------------------------------------------------------------------------------
        case LWS_CALLBACK_ESTABLISHED: {
          PWebSocketSession session = server->findSessionByWSI(wsi);
          server->onConnected(session);
          session->onConnected();
        }; break;


        // ------------------------------------------------------------------------------------------------------------
        // --- Data received
        // ------------------------------------------------------------------------------------------------------------
        case LWS_CALLBACK_RECEIVE: {
          PWebSocketSession session = server->findSessionByWSI(wsi);
          std::size_t remaining_payload_size  = lws_remaining_packet_payload(wsi);
          bool        is_final_fragment       = lws_is_final_fragment(wsi);
          server->onReceive(session, reinterpret_cast<const std::uint8_t*>(payload), payload_size, remaining_payload_size, is_final_fragment);
          session->onReceive(reinterpret_cast<const std::uint8_t*>(payload), payload_size, remaining_payload_size, is_final_fragment);
        }; break;

        // ------------------------------------------------------------------------------------------------------------
        // --- Server become writtable
        // ------------------------------------------------------------------------------------------------------------
        case LWS_CALLBACK_SERVER_WRITEABLE: {
          PWebSocketSession session = server->findSessionByWSI(wsi);
          session->__onWrittable__();
        }; break;

        // ------------------------------------------------------------------------------------------------------------
        // --- Close session request received
        // ------------------------------------------------------------------------------------------------------------
        case LWS_CALLBACK_WS_PEER_INITIATED_CLOSE: {
          PWebSocketSession session = server->findSessionByWSI(wsi);
          session->__shutdown__();
          return session->onDisconnectRequested() ? 0 : 1;
        }; break;

        // ------------------------------------------------------------------------------------------------------------
        // --- Connection closed
        // ------------------------------------------------------------------------------------------------------------
        case LWS_CALLBACK_CLOSED: {
          PWebSocketSession session = server->findSessionByWSI(wsi);
          session->__shutdown__();
          server->onDisconnected(session);
          session->onDisconnected();
        }; break;

        // ------------------------------------------------------------------------------------------------------------
        // --- Destroy session
        // ------------------------------------------------------------------------------------------------------------
        case LWS_CALLBACK_WSI_DESTROY:
          server->destorySession(wsi);
          break;

        // ------------------------------------------------------------------------------------------------------------
        // --- Unknown callback
        // ------------------------------------------------------------------------------------------------------------
        default:
          printf("*** Not implemented WebSocket callback reason: %ld\n", (std::size_t)reason);
          break;
      }
      return 0;
    }

  public:

    //! Constructor
    WebSocketServer()
    : WebSocket(Mode::Server)
    , _update_timeout(10)
    #ifdef __WITH_ACTIVITY__
    , _server_activity(this, &WebSocketServer::__serverActivity__)
    #endif
    {
      // - Release context info
      memset(&_context_options, 0x00, sizeof(_context_options));

      // - Set generic context options
      getContextOptions()->user = this;
    }

    //! Destructor
    ~WebSocketServer() {
      stop();
    }

    #ifdef __WITH_ACTIVITY__
  private:
    //! Server activity
    void __serverActivity__() {
      while (_server_activity.running()) try {
        _server_activity.__cancel_point__();
        update();
      } catch (const std::exception& e) {}
    }
    #endif

  public:

    //! Add new protocol
    bool addProtocol(const WebSocketProtocol& protocol) {
      // - Check is protocol already exists
      if (_protocols.find(protocol.getName()) != _protocols.end()) return false;
      _protocols[protocol.getName()] = protocol;
      return true;
    }

    //! Get protocol by name
    WebSocketProtocol* getProtocol(const std::string& name) {
      // - Find protocol
      auto i_protocol = _protocols.find(name);
      if (i_protocol == _protocols.end()) return nullptr;
      else return &i_protocol->second;
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
      getContextOptions()->ka_interval = timeout_sec;
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

    //! Set update timeout
    void setUpdateTimeout(std::uint32_t timeout) {
      _update_timeout = timeout;
    }

    //! Get update timeout
    std::uint32_t getUpdateTimeout() const {
      return _update_timeout;
    }

  public:
    //! On server start handler
    virtual void onStart()
    {}

    //! On server stop handler
    virtual void onStop()
    {}

  public:

    //! Start WebSocket server
    bool start() override {
      // - Check state
      if (getRunningState() != RunningState::Stopped) return false;
      setRunningState(RunningState::Starting);

      // - Check is protocols set
      if (_protocols.empty()) {
        setRunningState(RunningState::Stopped);
        return false;
      }

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
      if (!getContext()) {
        setRunningState(RunningState::Stopped);
        return false;
      } else {
        // - Started
        onStart();
        setRunningState(RunningState::Running);
        return true;
      }
    }

    #ifdef __WITH_ACTIVITY__
    //! Start standalone server with own thread
    bool startStandalone() {
      if (!start()) return false;

      // - Run activity
      _server_activity.start();
      return true;
    }
    #endif

    //! Update server
    void update() override {
      // - Update only running server
      if (getRunningState() != RunningState::Running) return;

      // - Update LWS
      lws_service(getContext(), _update_timeout);
    }

    //! Stop server
    bool stop() override {
      // - Stop only running server
      if (getRunningState() != RunningState::Running) return false;
      setRunningState(RunningState::Stopping);

      #ifdef __WITH_ACTIVITY__
      _server_activity.stop();
      #endif

      // - Disconnect all active sessions
      destoryContext();

      // - Cleanup
      _protocols_plain.clear();

      // - Stopped
      onStop();
      setRunningState(RunningState::Stopped);
      return true;
    }

  public:

    //! Get active sessions
    std::vector<PWebSocketSession> getActiveSessions() {
      std::lock_guard<std::mutex> lock(_sessions_mutex);
      std::vector<PWebSocketSession> result;
      for (auto& i_session : _sessions_by_id) {
        result.push_back(i_session.second);
      }
      return std::move(result);
    }
  };

  //! WebSocket client
  class WebSocketClient : public WebSocket {

  private:
    //! Constructor
    WebSocketClient() {


    }
  };


}
