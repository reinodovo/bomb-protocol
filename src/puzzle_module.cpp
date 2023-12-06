#include <map>
#include <set>

#include <puzzle_module.h>
#include <utils/debouncer.h>

namespace PuzzleModule {
const int STATUS_LIGHT_STRIKE_DURATION = 1000;
const int STATUS_LIGHT_STRIKE_BLINK_DURATION = 1000;
unsigned long _last_strike;

const int BOMB_INFO_DELAY = 50;
unsigned long _last_bomb_info_request = 0;
int _bomb_info_key_index = 0;
std::map<int, BombInfoCallback> _bomb_info_callbacks_map;

String _mac_address;
esp_now_peer_info_t _main_module;

bool _connected, _started, _solved;

const int SOLVE_ATTEMPT_DELAY = 50;
int _solve_attempt_key_index = 0;
std::map<int, SolveAttempt> _pending_solve_attempts;
Debouncer _solve_attempt_debouncer(SOLVE_ATTEMPT_DELAY);

int _code;

OnRestart onRestart = nullptr;
OnStart onStart = nullptr;
OnManualCode onManualCode = nullptr;

StatusLight _light;

void sendPendingSolveAttempts() {
  if (_pending_solve_attempts.empty())
    return;
  SolveAttempt sa = _pending_solve_attempts.begin()->second;
  send(sa, _main_module.peer_addr);
}

void queueSolveAttempt(bool strike) {
  SolveAttempt info;
  info.strike = strike;
  info.key = _solve_attempt_key_index++;
  _pending_solve_attempts[info.key] = info;
}

void solveAttemptAckRecv(SolveAttemptAck ack) {
  if (_pending_solve_attempts.find(ack.key) == _pending_solve_attempts.end())
    return;
  _pending_solve_attempts.erase(ack.key);
}

void strike() {
  _light.strike();
  _last_strike = millis();
  queueSolveAttempt(true);
}

void solve() {
  _solved = true;
  queueSolveAttempt(false);
}

void withBombInfo(BombInfoCallback callback) {
  BombInfoRequest info;
  info.key = _bomb_info_key_index++;
  _bomb_info_callbacks_map[info.key] = callback;
  send(info, _main_module.peer_addr);
}

void bombInfoRecv(BombInfo info) {
  if (_bomb_info_callbacks_map.find(info.request_key) ==
      _bomb_info_callbacks_map.end())
    return;
  _bomb_info_callbacks_map[info.request_key](info);
  _bomb_info_callbacks_map.erase(info.request_key);
}

ModuleStatus status() {
  if (!_connected)
    return ModuleStatus::Connecting;
  if (!_started)
    return ModuleStatus::Connected;
  if (!_solved)
    return ModuleStatus::Started;
  return ModuleStatus::Solved;
}

void connectionInfoRecv(Connection info, const uint8_t *mac) {
  if (!_connected && tryConnectingToPeer(mac, &_main_module))
    _connected = true;
  else
    return;
  strcpy(info.mac_address, _mac_address.c_str());
  send(info, _main_module.peer_addr);
}

void startRecv() {
  if (_code == -1)
    return;
  if (!_started && onStart != nullptr)
    onStart();
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

void updateStatusLight() {
  if (millis() - _last_strike < STATUS_LIGHT_STRIKE_BLINK_DURATION)
    return;
  _light.update(status());
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

void update() {
  updateManualCode();
  updateStatusLight();
  _solve_attempt_debouncer([&]() { sendPendingSolveAttempts(); });
}

bool setup(StatusLight light) {
  initialize();

  _light = light;

  Callbacks callbacks;
  callbacks.bombInfoCallback = bombInfoRecv;
  callbacks.connectionCallback = connectionInfoRecv;
  callbacks.solveAttemptAckCallback = solveAttemptAckRecv;
  callbacks.startCallback = startRecv;
  callbacks.resetCallback = resetRecv;

  if (!initProtocol(callbacks))
    return false;

  _mac_address = WiFi.macAddress();

  return true;
}

StatusLight::StatusLight(int redPin, int greenPin)
    : _redPin(redPin), _greenPin(greenPin) {
  pinMode(_redPin, OUTPUT);
  pinMode(_greenPin, OUTPUT);
}

void StatusLight::update(ModuleStatus status) {
  if (status == ModuleStatus::Connecting) {
    bool state = (millis() / STATUS_LIGHT_STRIKE_BLINK_DURATION) % 2;
    digitalWrite(_redPin, state);
    digitalWrite(_greenPin, state);
  } else if (status == ModuleStatus::Connected) {
    digitalWrite(_redPin, LOW);
    digitalWrite(_greenPin, LOW);
  } else if (status == ModuleStatus::Started) {
    digitalWrite(_redPin, HIGH);
    digitalWrite(_greenPin, HIGH);
  } else if (status == ModuleStatus::Solved) {
    digitalWrite(_redPin, HIGH);
    digitalWrite(_greenPin, LOW);
  }
}

void StatusLight::strike() {
  digitalWrite(_redPin, LOW);
  digitalWrite(_greenPin, HIGH);
}
} // namespace PuzzleModule