#include <map>
#include <set>

#include <module.h>
#include <ota.h>
#include <utils/debouncer.h>

namespace Module {
ModuleType _type;

const int BOMB_INFO_DELAY = 50;
unsigned long _last_bomb_info_request = 0;
int _bomb_info_key_index = 0;
std::map<int, BombInfoCallback> _bomb_info_callbacks_map;
Debouncer _bomb_info_debouncer(BOMB_INFO_DELAY);

const int UPDATE_MANUAL_CODE_DELAY = 200;
Debouncer _update_manual_code_debouncer(UPDATE_MANUAL_CODE_DELAY);

String _mac_address;
esp_now_peer_info_t _main_module;

bool _connected, _started, _solved;

const int SOLVE_ATTEMPT_DELAY = 50;
int _solve_attempt_key_index = 0;
std::map<int, SolveAttempt> _pending_solve_attempts;
Debouncer _solve_attempt_debouncer(SOLVE_ATTEMPT_DELAY);

int _code;

String name = "Unknown";
OnRestart onRestart = nullptr;
OnStart onStart = nullptr;
OnManualCode onManualCode = nullptr;

void sendPendingSolveAttempts() {
  if (_pending_solve_attempts.empty())
    return;
  SolveAttempt sa = _pending_solve_attempts.begin()->second;
  send(sa, _main_module.peer_addr);
}

void queueSolveAttempt(SolveAttempt attempt) {
  attempt.key = _solve_attempt_key_index++;
  _pending_solve_attempts[attempt.key] = attempt;
}

void solveAttemptAckRecv(SolveAttemptAck ack) {
  if (_pending_solve_attempts.find(ack.key) == _pending_solve_attempts.end())
    return;
  _pending_solve_attempts.erase(ack.key);
}

void withBombInfo(BombInfoCallback callback) {
  int key = _bomb_info_key_index++;
  _bomb_info_callbacks_map[key] = callback;
}

void bombInfoRecv(BombInfo info) {
  for (auto _bomb_info_callback : _bomb_info_callbacks_map) {
    _bomb_info_callback.second(info);
    _bomb_info_callbacks_map.erase(_bomb_info_callback.first);
  }
}

Status status() {
  if (OTA::running())
    return Status::OTA;
  if (!_connected)
    return Status::Connecting;
  if (!_started)
    return Status::Connected;
  if (!_solved)
    return Status::Started;
  return Status::Solved;
}

void connectionInfoRecv(Connection info, const uint8_t *mac) {
  if (!_connected && tryConnectingToPeer(mac, &_main_module)) {
    if (DEBUG)
      Serial.println("Connected to main module");
    _connected = true;
  }
}

void startRecv() {
  if (_code == -1)
    return;
  if (!_started && onStart != nullptr) {
    if (DEBUG)
      Serial.println("Starting module");
    onStart();
  }
  _started = true;
  send(START_ACK, _main_module.peer_addr);
}

void initialize() {
  _code = -1;
  _connected = _started = _solved = false;
  _pending_solve_attempts.clear();
  _bomb_info_callbacks_map.clear();
}

void resetRecv() {
  initialize();
  _connected = true;
  if (onRestart != nullptr)
    onRestart();
  send(RESET_ACK, _main_module.peer_addr);
}

void updateManualCode() {
  if (onManualCode != nullptr &&
      millis() - _last_bomb_info_request > BOMB_INFO_DELAY) {
    _last_bomb_info_request = millis();
    withBombInfo([](BombInfo info) {
      if (info.code != _code) {
        _code = info.code;
        onManualCode(_code);
      }
    });
  }
}

void solve() { _solved = true; }

void update() {
  OTA::update();

  _update_manual_code_debouncer(updateManualCode);
  if (!_bomb_info_callbacks_map.empty())
    _bomb_info_debouncer([&]() {
      BombInfoRequest info;
      info.key = 0;
      send(info, _main_module.peer_addr);
    });
  _solve_attempt_debouncer([&]() { sendPendingSolveAttempts(); });
}

bool setup(ModuleType type) {
  initialize();

  _type = type;

  Callbacks callbacks;
  callbacks.bombInfoCallback = bombInfoRecv;
  callbacks.connectionCallback = connectionInfoRecv;
  callbacks.solveAttemptAckCallback = solveAttemptAckRecv;
  callbacks.startCallback = startRecv;
  callbacks.resetCallback = resetRecv;

  if (!initProtocol(name, callbacks, _type))
    return false;

  if (OTA::running())
    return true;

  _mac_address = WiFi.macAddress();

  return true;
}
} // namespace Module