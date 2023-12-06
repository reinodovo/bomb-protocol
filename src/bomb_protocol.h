#ifndef BOMB_PROTOCOL_H
#define BOMB_PROTOCOL_H

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <functional>

const int MAC_ADDRESS_SIZE = 18;

typedef struct BombInfo {
  int request_key;
  char time[6];
  int strikes;
  int code;
} BombInfo;

typedef struct BombInfoRequest {
  int key;
} BombInfoRequest;

typedef struct Connection {
  char mac_address[MAC_ADDRESS_SIZE];
} Connection;

typedef struct SolveAttempt {
  bool strike;
  int key;
} SolveAttempt;

typedef struct SolveAttemptAck {
  bool strike;
  int key;
} SolveAttemptAck;

enum MessageType {
  UNKNOWN,
  CONNECTION,
  BOMB_INFO,
  BOMB_INFO_REQUEST,
  SOLVE_ATTEMPT,
  SOLVE_ATTEMPT_ACK,
  START,
  START_ACK,
  RESET,
  RESET_ACK,
  HEARTBEAT,
  HEARTBEAT_ACK,
};

using ConnectionCallback =
    std::function<void(Connection info, const uint8_t *mac)>;
using BombInfoCallback = std::function<void(BombInfo info)>;
using BombInfoRequestCallback =
    std::function<void(BombInfoRequest info, const uint8_t *mac)>;
using SolveAttemptCallback =
    std::function<void(SolveAttempt info, const uint8_t *mac)>;
using SolveAttemptAckCallback = std::function<void(SolveAttemptAck info)>;
using StartCallback = std::function<void()>;
using StartAckCallback = std::function<void(const uint8_t *mac)>;
using ResetCallback = std::function<void()>;
using ResetAckCallback = std::function<void(const uint8_t *mac)>;
using HeartbeatAckCallback = std::function<void(const uint8_t *mac)>;

using Send = std::function<esp_err_t()>;

typedef struct Callbacks {
  ConnectionCallback connectionCallback;
  BombInfoCallback bombInfoCallback;
  BombInfoRequestCallback bombInfoRequestCallback;
  SolveAttemptCallback solveAttemptCallback;
  SolveAttemptAckCallback solveAttemptAckCallback;
  StartCallback startCallback;
  StartAckCallback startAckCallback;
  ResetCallback resetCallback;
  ResetAckCallback resetAckCallback;
  HeartbeatAckCallback heartbeatAckCallback;
} Callbacks;

bool initProtocol(Callbacks);

bool tryConnectingToPeer(const uint8_t *mac, esp_now_peer_info_t *peer);

esp_err_t send(MessageType type, const uint8_t *mac);
esp_err_t send(Connection info, const uint8_t *mac);
esp_err_t send(BombInfo info, const uint8_t *mac);
esp_err_t send(BombInfoRequest info, const uint8_t *mac);
esp_err_t send(SolveAttempt info, const uint8_t *mac);
esp_err_t send(SolveAttemptAck info, const uint8_t *mac);

#endif