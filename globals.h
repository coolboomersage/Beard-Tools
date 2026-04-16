#pragma once

#include <atomic>
#include <mutex>
#include <string>

#include "types.h"
#include "specData.h"

// ---------------------------------------------------------------------------
// Report data shared between the main thread and background fetch thread
// ---------------------------------------------------------------------------
extern ReportData        g_report;
extern std::mutex        g_reportMutex;
extern std::atomic<bool> g_reportReady;

// Cached after the first successful report fetch so player-button clicks
// can re-use the same credentials without threading headaches.
extern std::string       g_cachedReportCode;
extern std::string       g_cachedBearerToken;

// ---------------------------------------------------------------------------
// Per-player debug/detail window state
// ---------------------------------------------------------------------------
extern Fight             g_currentFight;
extern bool              g_fetchedBossData;
extern std::mutex        g_debugMutex;
extern bool              g_debugWindowOpen;
extern std::atomic<bool> g_debugFetching;
extern std::string       g_debugWindowTitle;  // "PlayerName — FightName"
extern std::string       g_debugRawResponse;  // verbatim server JSON
extern int               g_debugPlayerId;
inline std::string       g_cachedBossJson;
// Module-level singletons — written once per fetch, read every frame.
static PlayerWindowData  g_playerData;
static bool              g_playerDataReady = false;
// Edge-detect flag: set true when a player button is clicked,
// cleared once BuildPlayerData() has run after the fetch completes.
static bool              g_wasFetching     = false;

// ---------------------------------------------------------------------------
// Spec map — maps WCL class/spec strings to hardcoded WoWclass data.
// The array size (39) must match the definition in globals.cpp.
// ---------------------------------------------------------------------------
extern AllSpecs          masterSpecList;
extern const SpecEntry   kSpecMap[39];