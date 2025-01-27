#include <main_module.h>
#include <map>
#include <ota.h>
#include <set>
#include <utils/debouncer.h>

namespace MainModule {
const int MAX_CODE = 9999;

const unsigned long ONE_SECOND = 1000;
const unsigned long ONE_MINUTE = 60 * ONE_SECOND;

String mac_address;
esp_now_peer_info_t broadcast;
esp_now_peer_info_t modules[MAX_MODULES];
bool modules_solved[MAX_MODULES];
bool modules_started[MAX_MODULES];
bool modules_reset[MAX_MODULES];
ModuleType modules_types[MAX_MODULES];
int modules_connected = 0;

OnSolved onSolved = nullptr;
OnFailed onFailed = nullptr;
OnStrike onStrike = nullptr;

bool _should_reset;
unsigned long _should_start_at = 0;

bool _started;
bool _solved;
bool _failed;

int _strikes;
int _max_strikes;

int _code;

unsigned long _duration;
unsigned long _start_time;
unsigned long _elapsed_time[SPEED_STAGES];
unsigned long _last_update_time;

const int BROADCAST_DEBOUNCE_DELAY = 1000;
Debouncer broadcast_debouncer(BROADCAST_DEBOUNCE_DELAY);
const int START_DEBOUNCE_QUICK_DELAY = 50;
const int START_DEBOUNCE_SLOW_DELAY = 500;
Debouncer start_debouncer_quick(START_DEBOUNCE_QUICK_DELAY);
Debouncer start_debouncer_slow(START_DEBOUNCE_SLOW_DELAY);
const int RESET_DEBOUNCE_DELAY = 50;
Debouncer reset_debouncer(RESET_DEBOUNCE_DELAY);
const int HEARTBEAT_DEBOUNCE_DELAY = 50;
Debouncer heartbeat_debouncer(HEARTBEAT_DEBOUNCE_DELAY);

std::map<int, std::set<int>> _pending_solve_attempts;

bool compareMacAddress(const uint8_t *mac1, const uint8_t *mac2) {
  for (int i = 0; i < 6; i++)
    if (mac1[i] != mac2[i])
      return false;
  return true;
}

int findMacAddress(const uint8_t *mac) {
  for (int i = 0; i < modules_connected; i++)
    if (compareMacAddress(mac, modules[i].peer_addr))
      return i;
  return -1;
}

unsigned long elapsedTime() {
  unsigned long elapsed = 0;
  unsigned long speed_stages = SPEED_STAGES;
  for (unsigned long i = 0; i < SPEED_STAGES; i++)
    elapsed += (_elapsed_time[i] * speed_stages) / (speed_stages - i);
  return min(elapsed, _duration);
}

bool started() { return _started; }

bool starting() {
  return !started() && _should_start_at != 0 && millis() > _should_start_at;
}

bool onStartCountdown() {
  return !started() && !starting() && _should_start_at != 0;
}

int strikes() { return _strikes; }

int maxStrikes() { return _max_strikes; }

bool solved() { return _solved; }

void solve() {
  if (failed() || solved())
    return;
  if (onSolved != nullptr)
    onSolved();
  _solved = true;
}

bool failed() { return _failed; }

void fail() {
  if (failed() || solved())
    return;
  if (onFailed != nullptr)
    onFailed();
  _failed = true;
}

void updateMissingTime() {
  if (!started() || solved() || failed())
    return;
  unsigned long current_time = millis();
  _elapsed_time[speed()] += current_time - _last_update_time;
  _last_update_time = current_time;
  if (elapsedTime() >= _duration)
    fail();
}

char *remainingTimeString(unsigned long elapsed, unsigned long duration,
                          bool show_millis = true) {
  unsigned long remaining = duration - elapsed;
  int minutes = remaining / ONE_MINUTE;
  int seconds = (remaining % ONE_MINUTE) / ONE_SECOND;
  int milliseconds = (remaining % ONE_SECOND) / 10;
  char *result = new char[6];
  if (minutes == 0 && show_millis)
    sprintf(result, "%02d.%02d", seconds, milliseconds);
  else
    sprintf(result, "%02d:%02d", minutes, seconds);
  return result;
}

char *timeStrToStart() {
  return remainingTimeString(millis(), max(millis(), _should_start_at), true);
}

char *timeStr(bool show_millis = true) {
  return remainingTimeString(elapsedTime(), _duration, show_millis);
}

int code() { return _code; }

BombInfo bombInfo() {
  BombInfo info;
  char *str = timeStr();
  strcpy(info.time, str);
  delete[] str;
  info.strikes = _strikes;
  info.max_strikes = _max_strikes;
  info.code = _code;
  info.failed = failed();
  info.solved = solved();
  info.total_puzzle_modules = 0;
  info.solved_puzzle_modules = 0;
  info.total_needy_modules = 0;
  for (int i = 0; i < modules_connected; i++) {
    if (modules_types[i] == Puzzle) {
      info.total_puzzle_modules++;
      if (modules_solved[i])
        info.solved_puzzle_modules++;
    }
    if (modules_types[i] == Needy)
      info.total_needy_modules++;
  }
  return info;
}

void bombInfoRequestRecv(BombInfoRequest req, const uint8_t *mac) {
  BombInfo info = bombInfo();
  info.request_key = req.key;
  send(info, mac);
}

void sendSolveAttemptAck(SolveAttempt info, const uint8_t *mac) {
  SolveAttemptAck ack;
  ack.strike = info.strike;
  ack.key = info.key;
  send(ack, mac);
}

bool isSolveAttemptPending(SolveAttempt info, int module_index) {
  if (_pending_solve_attempts.find(module_index) ==
      _pending_solve_attempts.end())
    return true;
  if (_pending_solve_attempts[module_index].find(info.key) ==
      _pending_solve_attempts[module_index].end())
    return true;
  return false;
}

void strike() {
  _strikes = min(++_strikes, _max_strikes);
  if (_strikes >= _max_strikes)
    fail();
  if (onStrike != nullptr)
    onStrike(_strikes);
}

void solveAttemptRecv(SolveAttempt info, const uint8_t *mac) {
  sendSolveAttemptAck(info, mac);
  int module_index = findMacAddress(mac);
  if (module_index == -1)
    return;
  if (!isSolveAttemptPending(info, module_index))
    return;
  _pending_solve_attempts[module_index].insert(info.key);
  if (info.strike) {
    strike();
    return;
  }
  if (info.fail) {
    fail();
    return;
  }
  modules_solved[module_index] = true;
  bool is_solved = true;
  for (int i = 0; i < modules_connected; i++)
    if (!modules_solved[i] && modules_types[i] == Puzzle)
      is_solved = false;
  if (is_solved)
    solve();
}

void resetAckRecv(const uint8_t *mac) {
  int module_index = findMacAddress(mac);
  if (module_index == -1 || modules_reset[module_index])
    return;
  modules_reset[module_index] = true;
  bool all_modules_reset = true;
  for (int i = 0; i < modules_connected; i++)
    if (!modules_reset[i])
      all_modules_reset = false;
  if (!all_modules_reset)
    return;
  _should_reset = false;
}

void startAckRecv(const uint8_t *mac) {
  if (started())
    return;
  int module_index = findMacAddress(mac);
  if (module_index == -1 || modules_started[module_index])
    return;
  modules_started[module_index] = true;
  bool all_modules_started = true;
  for (int i = 0; i < modules_connected; i++)
    if (!modules_started[i])
      all_modules_started = false;
  if (!all_modules_started)
    return;
  _start_time = millis();
  _last_update_time = millis();
  _started = true;
}

void heartbeatAckRecv(ModuleType type, const uint8_t *mac) {
  if (findMacAddress(mac) != -1)
    return;
  if (!tryConnectingToPeer(mac, &modules[modules_connected++]))
    modules_connected--;
  else
    modules_types[modules_connected - 1] = type;
}

void setMaxStrikes(int max_strikes) { _max_strikes = max_strikes; }

void setDuration(unsigned long duration) { _duration = duration; }

void initialize() {
  for (int i = 0; i < modules_connected; i++)
    esp_now_del_peer(modules[i].peer_addr);
  modules_connected = 0;

  _should_reset = false;

  _should_start_at = 0;
  _started = false;

  _strikes = 0;

  _solved = false;
  _failed = false;
  _code = esp_random() % (MAX_CODE + 1);

  for (int i = 0; i < SPEED_STAGES; i++)
    _elapsed_time[i] = 0;

  for (int i = 0; i < MAX_MODULES; i++) {
    modules_solved[i] = false;
    modules_started[i] = false;
    modules_reset[i] = false;
  }

  _pending_solve_attempts.clear();
}

bool setup() {
  initialize();

  Callbacks callbacks;
  callbacks.bombInfoRequestCallback = bombInfoRequestRecv;
  callbacks.solveAttemptCallback = solveAttemptRecv;
  callbacks.startAckCallback = startAckRecv;
  callbacks.resetAckCallback = resetAckRecv;
  callbacks.heartbeatAckCallback = heartbeatAckRecv;

  if (!initProtocol("Main Module", callbacks, Main))
    return false;

  mac_address = WiFi.macAddress();

  uint8_t broadcastAddress[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
  if (!tryConnectingToPeer(broadcastAddress, &broadcast))
    return false;

  return true;
}

void startAfter(int seconds) {
  _should_start_at = millis() + seconds * ONE_SECOND;
}

void reset() {
  initialize();
  _should_reset = true;
}

esp_err_t broadcastMacAddress() {
  Connection info;
  strcpy(info.mac_address, mac_address.c_str());
  return send(info, broadcast.peer_addr);
}

void update() {
  OTA::update();

  updateMissingTime();
  broadcast_debouncer([&]() { broadcastMacAddress(); });
  if (onStartCountdown())
    heartbeat_debouncer([&]() { send(HEARTBEAT, broadcast.peer_addr); });
  if (starting())
    start_debouncer_quick([&]() { send(START, broadcast.peer_addr); });
  if (started())
    start_debouncer_slow([&]() { send(START, broadcast.peer_addr); });
  if (_should_reset)
    reset_debouncer([&]() { send(RESET, broadcast.peer_addr); });
}

int speed() { return min(_strikes, SPEED_STAGES - 1); }

unsigned long timeToNextSecond() {
  unsigned long remaining = ONE_SECOND - (elapsedTime() % ONE_SECOND);
  return (remaining * (SPEED_STAGES - speed())) / SPEED_STAGES;
}
} // namespace MainModule