#pragma once

#ifdef DEVKIT_AAM

#include <Arduino.h>

struct PoolFooterData {
  String workersCount;
  String workersHash;
  String bestDifficulty;
};

struct MiningScreenData {
  String totalMHashes;
  String templates;
  String bestDiff;
  String completedShares;
  String timeMining;
  String valids;
  String temp;
  String currentTime;
  String currentHashRate;
};

struct ClockScreenData {
  String currentHashRate;
  String blockHeight;
  String btcPrice;
  String currentTime;
};

struct GlobalScreenData {
  String btcPrice;
  String currentTime;
  String halfHourFee;
  String networkDifficulty;
  String globalHashRate;
  String remainingBlocks;
  String blockHeight;
  float progressPercent;
};

struct PriceScreenData {
  String currentHashRate;
  String blockHeight;
  String currentTime;
  String btcPrice;
};

struct RemoteScreenData {
  String board;
  String hashRate;
  String shares;
  String bestDiff;
  String valid;
  String rssi;
  bool connected;
  String currentTime;
  String timeMining;
};

struct AAMDisplayData {
  PoolFooterData pool;
  MiningScreenData mining;
  ClockScreenData clock;
  GlobalScreenData global;
  PriceScreenData price;
  RemoteScreenData remote;
};

void aamDisplay_SetData(const AAMDisplayData &data);
const AAMDisplayData &aamDisplay_GetData();

#endif
