#pragma once

#include <string>
#include <vector>
#include <optional>

#include "external/imgui-1.92.7/imgui.h"
#include "specData.h"

// ── WoW item upgrade track bonus IDs -> display colour ───────────────────────
// These bonus IDs are season-specific. Update each new season/patch.
// Season 1 - Midnight (patch 12.0)
struct UpgradeTrack {
    int         bonusID;
    const char* name;
    ImVec4      color;
};

// These are the rank-family IDs that encode both track and rank.
// 1279x = Hero track, 1280x = Myth track, 1281x = unknown (possibly Hero too)
// From observed data:
//   12794 = Hero 2/6,  12795 = Hero 3/6,  12796 = Hero 4/6
//   12797 = Hero 5/6,  12798 = Hero 6/6
//   12799 = Veteran?   12800 = Veteran?    12801 = Champ?
//   12802 = Myth 2/6,  12803 = Myth 3/6,  12804 = Myth 4/6
//   12805 = Myth 5/6,  12806 = Myth 6/6
// Lower rank IDs per track are inferred — only 12794-12798 and 12802/12806 confirmed.
// The 12699 ID appears on Hero items without a clear rank, possibly an older season marker.
static const UpgradeTrack kUpgradeTracks[] = {
    // ── Hero track (12794-12798) ─────────────────────────────────────────
    { 12794, "Hero 2/6",    ImVec4(1.0f, 0.85f, 0.0f, 1.0f) },
    { 12795, "Hero 3/6",    ImVec4(1.0f, 0.85f, 0.0f, 1.0f) },
    { 12796, "Hero 4/6",    ImVec4(1.0f, 0.85f, 0.0f, 1.0f) },
    { 12797, "Hero 5/6",    ImVec4(1.0f, 0.85f, 0.0f, 1.0f) },
    { 12798, "Hero 6/6",    ImVec4(1.0f, 0.85f, 0.0f, 1.0f) },
    // ── Myth track (12802-12806) ─────────────────────────────────────────
    { 12802, "Myth 2/6",    ImVec4(0.4f, 1.0f, 0.9f, 1.0f) },
    { 12803, "Myth 3/6",    ImVec4(0.4f, 1.0f, 0.9f, 1.0f) },
    { 12804, "Myth 4/6",    ImVec4(0.4f, 1.0f, 0.9f, 1.0f) },
    { 12805, "Myth 5/6",    ImVec4(0.4f, 1.0f, 0.9f, 1.0f) },
    { 12806, "Myth 6/6",    ImVec4(0.4f, 1.0f, 0.9f, 1.0f) },
    // ── Champion track (12790-12793) — inferred, only 12790 confirmed ────
    { 12790, "Champ 6/6",   ImVec4(0.7f, 0.3f, 1.0f, 1.0f) },
    { 12791, "Champ ?/6",   ImVec4(0.7f, 0.3f, 1.0f, 1.0f) },
    { 12792, "Champ ?/6",   ImVec4(0.7f, 0.3f, 1.0f, 1.0f) },
    { 12793, "Champ ?/6",   ImVec4(0.7f, 0.3f, 1.0f, 1.0f) },
    // ── Veteran track (12785-12789) — fully inferred ─────────────────────
    { 12785, "Veteran ?/6", ImVec4(0.3f, 0.5f, 1.0f, 1.0f) },
    { 12786, "Veteran ?/6", ImVec4(0.3f, 0.5f, 1.0f, 1.0f) },
    { 12787, "Veteran ?/6", ImVec4(0.3f, 0.5f, 1.0f, 1.0f) },
    { 12788, "Veteran ?/6", ImVec4(0.3f, 0.5f, 1.0f, 1.0f) },
    { 12789, "Veteran ?/6", ImVec4(0.3f, 0.5f, 1.0f, 1.0f) },
    // ── Adventurer track (12780-12784) — fully inferred ──────────────────
    { 12780, "Adv ?/6",     ImVec4(0.1f, 0.8f, 0.1f, 1.0f) },
    { 12781, "Adv ?/6",     ImVec4(0.1f, 0.8f, 0.1f, 1.0f) },
    { 12782, "Adv ?/6",     ImVec4(0.1f, 0.8f, 0.1f, 1.0f) },
    { 12783, "Adv ?/6",     ImVec4(0.1f, 0.8f, 0.1f, 1.0f) },
    { 12784, "Adv ?/6",     ImVec4(0.1f, 0.8f, 0.1f, 1.0f) },
};

// ---------------------------------------------------------------------------
// Player entry — carries both the WCL numeric id and the display name so that
// buttons can pass the id straight to fetchPersonalFightData().
// ---------------------------------------------------------------------------
struct PlayerEntry {
    int         id;
    std::string name;
};

struct Fight {
    int         id;
    std::string name;
    double      startTime;
    double      endTime;
    std::optional<bool> kill;
    std::optional<int>  difficulty;
    float       percent;

    std::vector<PlayerEntry> tanks;
    std::vector<PlayerEntry> healers;
    std::vector<PlayerEntry> dps;
};

struct ReportData {
    std::string title;
    std::string owner;
    std::vector<Fight> fights;
};

// ---------------------------------------------------------------------------
// Spec map entry — maps WCL class/spec strings to WoWclass data pointers.
// ---------------------------------------------------------------------------
struct SpecEntry {
    const char*     cls;
    const char*     spec;
    const WoWclass* wc;
};

// ---------------------------------------------------------------------------
// structs to store player window information
// ---------------------------------------------------------------------------
struct GearSlot {
    int         slot      = -1;
    std::string name;
    int         itemLevel = 0;
    bool        isTier    = false;
    bool        hasTrack  = false;
    std::string trackName;
    ImVec4      color      = ImVec4(0.55f, 0.55f, 0.55f, 1.0f);
    ImVec4      trackColor = ImVec4(1.0f,  1.0f,  1.0f,  1.0f);
    bool        empty      = true;
};

struct BossRow {
    std::string         name;
    std::vector<double> ts;
};

struct PlayerWindowData {
    // Identity
    std::string playerName  = "Unknown";
    std::string playerClass = "Unknown";
    std::string playerSpec  = "Unknown";
    std::string playerRole  = "Unknown";
    float       ilvl        = 0.0f;

    // Primary stats
    int agility   = 0;
    int strength  = 0;
    int intellect = 0;
    int stamina   = 0;

    // Secondary stats
    int    crit        = 0;
    int    haste       = 0;
    int    mastery     = 0;
    int    versatility = 0;
    double pieValues[4] = {};   // Crit, Haste, Mastery, Versatility

    // Defensive stats
    int armor     = 0;
    int leech     = 0;
    int avoidance = 0;
    int speed     = 0;

    // Gear (slots 0-17)
    GearSlot gear[18];

    // Timeline
    bool                hasSpellData = false;
    bool                hasEventData = false;
    std::vector<double> rotTs, offTs, defTs, mobTs, utilTs;
    int                 unknownCount = 0;
    double              tMax         = 0.0;

    // Boss timeline
    std::vector<BossRow> bossRows;

    // Error / status
    bool        parseOk = false;
    std::string parseError;
};
