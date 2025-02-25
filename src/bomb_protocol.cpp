#include <bomb_protocol.h>
#include <ota.h>

String _module_name = "Unknown";

Callbacks _callbacks;
ModuleType _type;
bool _started = false;

void onDataRecv(const uint8_t *mac, const uint8_t *incoming_data, int len);
MessageType getMessageInfo(const uint8_t *incoming_data, int len);

bool initProtocol(String module_name, Callbacks callbacks, ModuleType type) {
  if (DEBUG) {
    Serial.begin(BAUD_RATE);
    Serial.println("Initializing protocol");
  }
  _module_name = module_name;
  if (OTA::shouldStart()) {
    if (DEBUG)
      Serial.println("Starting OTA");
    OTA::start(APP_VERSION, module_name);
    return true;
  }

  _callbacks = callbacks;
  _type = type;
  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK)
    return false;
  if (DEBUG)
    Serial.println("ESP Now initialized");
  _started = true;
  esp_now_register_recv_cb(onDataRecv);
  return true;
}

bool tryConnectingToPeer(const uint8_t *mac, esp_now_peer_info_t *peer) {
  memcpy(peer->peer_addr, mac, 6);
  peer->channel = 0;
  peer->encrypt = false;
  if (esp_now_add_peer(peer) != ESP_OK)
    return false;
  return true;
}

void onBombInfoRecv(const uint8_t *mac, const uint8_t *incoming_data, int len) {
  if (_callbacks.bombInfoCallback == nullptr)
    return;
  BombInfo info;
  memcpy(&info, incoming_data + 1, sizeof(info));
  _callbacks.bombInfoCallback(info);
}

void onBombInfoRequestRecv(const uint8_t *mac, const uint8_t *incoming_data,
                           int len) {
  if (_callbacks.bombInfoRequestCallback == nullptr)
    return;
  BombInfoRequest info;
  memcpy(&info, incoming_data + 1, sizeof(info));
  _callbacks.bombInfoRequestCallback(info, mac);
}

void onSolveAttemptRecv(const uint8_t *mac, const uint8_t *incoming_data,
                        int len) {
  if (_callbacks.solveAttemptCallback == nullptr)
    return;
  SolveAttempt info;
  memcpy(&info, incoming_data + 1, sizeof(info));
  _callbacks.solveAttemptCallback(info, mac);
}

void onSolveAttemptAckRecv(const uint8_t *mac, const uint8_t *incoming_data,
                           int len) {
  if (_callbacks.solveAttemptAckCallback == nullptr)
    return;
  SolveAttemptAck info;
  memcpy(&info, incoming_data + 1, sizeof(info));
  _callbacks.solveAttemptAckCallback(info);
}

void onConnectionInfoRecv(const uint8_t *mac, const uint8_t *incoming_data,
                          int len) {
  if (_callbacks.connectionCallback == nullptr)
    return;
  Connection info;
  memcpy(&info, incoming_data + 1, sizeof(info));
  _callbacks.connectionCallback(info, mac);
}

void onStartRecv(const uint8_t *mac, const uint8_t *incoming_data, int len) {
  if (_callbacks.startCallback == nullptr)
    return;
  _callbacks.startCallback();
}

void onStartAckRecv(const uint8_t *mac, const uint8_t *incomingData, int len) {
  if (_callbacks.startAckCallback == nullptr)
    return;
  _callbacks.startAckCallback(mac);
}

void onResetRecv(const uint8_t *mac, const uint8_t *incoming_data, int len) {
  if (_callbacks.resetCallback == nullptr)
    return;
  _callbacks.resetCallback();
}

void onResetAckRecv(const uint8_t *mac, const uint8_t *incoming_data, int len) {
  if (_callbacks.resetAckCallback == nullptr)
    return;
  _callbacks.resetAckCallback(mac);
}

void onHeartbeatRecv(const uint8_t *mac, const uint8_t *incoming_data,
                     int len) {
  send(HEARTBEAT_ACK, _type, mac);
}

void onHeartbeatAckRecv(const uint8_t *mac, const uint8_t *incoming_data,
                        int len) {
  ModuleType type = (ModuleType)incoming_data[1];
  if (_callbacks.heartbeatAckCallback == nullptr)
    return;
  _callbacks.heartbeatAckCallback(type, mac);
}

void onDataRecv(const uint8_t *mac, const uint8_t *incoming_data, int len) {
  MessageType type = getMessageInfo(incoming_data, len);
  switch (type) {
  case CONNECTION:
    onConnectionInfoRecv(mac, incoming_data, len);
    break;
  case BOMB_INFO:
    onBombInfoRecv(mac, incoming_data, len);
    break;
  case BOMB_INFO_REQUEST:
    onBombInfoRequestRecv(mac, incoming_data, len);
    break;
  case SOLVE_ATTEMPT:
    onSolveAttemptRecv(mac, incoming_data, len);
    break;
  case SOLVE_ATTEMPT_ACK:
    onSolveAttemptAckRecv(mac, incoming_data, len);
    break;
  case START:
    onStartRecv(mac, incoming_data, len);
    break;
  case START_ACK:
    onStartAckRecv(mac, incoming_data, len);
    break;
  case RESET:
    onResetRecv(mac, incoming_data, len);
    break;
  case RESET_ACK:
    onResetAckRecv(mac, incoming_data, len);
    break;
  case HEARTBEAT:
    onHeartbeatRecv(mac, incoming_data, len);
    break;
  case HEARTBEAT_ACK:
    onHeartbeatAckRecv(mac, incoming_data, len);
    break;
  default:
    break;
  }
}

MessageType getMessageInfo(const uint8_t *incoming_data, int len) {
  uint8_t type = incoming_data[0];
  switch (type) {
  case CONNECTION:
    return CONNECTION;
  case BOMB_INFO:
    return BOMB_INFO;
  case BOMB_INFO_REQUEST:
    return BOMB_INFO_REQUEST;
  case SOLVE_ATTEMPT:
    return SOLVE_ATTEMPT;
  case SOLVE_ATTEMPT_ACK:
    return SOLVE_ATTEMPT_ACK;
  case START:
    return START;
  case START_ACK:
    return START_ACK;
  case RESET:
    return RESET;
  case RESET_ACK:
    return RESET_ACK;
  case HEARTBEAT:
    return HEARTBEAT;
  case HEARTBEAT_ACK:
    return HEARTBEAT_ACK;
  default:
    return UNKNOWN;
  }
}

esp_err_t send(MessageType type, const uint8_t *mac) {
  if (!_started)
    return ESP_FAIL;
  uint8_t message[1];
  message[0] = type;
  return esp_now_send(mac, message, sizeof(message));
}

template <typename T>
esp_err_t send(MessageType type, const T &info, const uint8_t *mac) {
  if (!_started)
    return ESP_FAIL;
  uint8_t message[sizeof(info) + 1];
  message[0] = type;
  memcpy(message + 1, &info, sizeof(info));
  return esp_now_send(mac, message, sizeof(message));
}

esp_err_t send(Connection info, const uint8_t *mac) {
  return send(CONNECTION, info, mac);
}

esp_err_t send(BombInfoRequest info, const uint8_t *mac) {
  return send(BOMB_INFO_REQUEST, info, mac);
}

esp_err_t send(BombInfo info, const uint8_t *mac) {
  return send(BOMB_INFO, info, mac);
}

esp_err_t send(SolveAttempt info, const uint8_t *mac) {
  return send(SOLVE_ATTEMPT, info, mac);
}

esp_err_t send(SolveAttemptAck info, const uint8_t *mac) {
  return send(SOLVE_ATTEMPT_ACK, info, mac);
}

esp_err_t send(MessageType type, ModuleType module_type, const uint8_t *mac) {
  return send<ModuleType>(type, module_type, mac);
}