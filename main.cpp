#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <unordered_set>

#include "types.h"
#include "globals.h"
#include "utils.h"
#include "api.h"

#include "external/imgui-1.92.7/imgui.h"
#include "external/imgui-1.92.7/backends/imgui_impl_glfw.h"
#include "external/imgui-1.92.7/backends/imgui_impl_opengl3.h"
#include "external/glfw-3.4/include/GLFW/glfw3.h"
#include "external/nlohmann/json.hpp"

#include "imPlotWrapper.h"

using json = nlohmann::json;

// ── BuildPlayerData: parse raw JSON strings -> PlayerWindowData ────────────────
// Called ONCE when g_debugFetching transitions false
static PlayerWindowData BuildPlayerData(
    const std::string& rawResponse,
    int                playerId,
    const std::string& bossJson)
{
    PlayerWindowData d;

    // Upgrade-track colour helper (runs during build, not during render)
    auto trackColor = [&](const json& item, ImVec4& outCol,
                          std::string& outName) -> bool {
        if (!item.contains("bonusIDs") || !item["bonusIDs"].is_array())
            return false;
        for (auto& bid : item["bonusIDs"]) {
            if (!bid.is_number_integer()) continue;
            int id = bid.get<int>();
            for (auto& track : kUpgradeTracks) {
                if (track.bonusID == id) {
                    outCol  = track.color;
                    outName = track.name;
                    return true;
                }
            }
        }
        return false;
    };

    try {
        auto root       = json::parse(rawResponse);
        auto& tableData = root["table"]["data"];
        auto& info      = tableData["combatantInfo"];
        auto& stats     = info["stats"];

        // ── Player identity ───────────────────────────────────────────────────
        if (tableData.contains("composition") &&
            tableData["composition"].is_array()) {
            for (auto& member : tableData["composition"]) {
                if (!member.contains("id") || !member["id"].is_number()) continue;
                if (member["id"].get<int>() != playerId) continue;

                d.playerName  = member.value("name", "Unknown");
                d.playerClass = member.value("type",  "Unknown");

                if (member.contains("specs") &&
                    member["specs"].is_array() &&
                    !member["specs"].empty()) {
                    auto& spec   = member["specs"][0];
                    d.playerSpec = spec.value("spec", "Unknown");
                    d.playerRole = spec.value("role", "Unknown");
                }
                break;
            }
        }

        d.ilvl = tableData.value("itemLevel", 0.0f);

        // ── Stats ─────────────────────────────────────────────────────────────
        auto getStat = [&](const char* key) -> int {
            if (stats.contains(key) && stats[key].contains("max"))
                return stats[key]["max"].get<int>();
            return 0;
        };

        d.agility     = getStat("Agility");
        d.strength    = getStat("Strength");
        d.intellect   = getStat("Intellect");
        d.stamina     = getStat("Stamina");
        d.crit        = getStat("Crit");
        d.haste       = getStat("Haste");
        d.mastery     = getStat("Mastery");
        d.versatility = getStat("Versatility");
        d.armor       = getStat("Armor");
        d.leech       = getStat("Leech");
        d.avoidance   = getStat("Avoidance");
        d.speed       = getStat("Speed");

        d.pieValues[0] = d.crit;
        d.pieValues[1] = d.haste;
        d.pieValues[2] = d.mastery;
        d.pieValues[3] = d.versatility;

        // ── Gear ──────────────────────────────────────────────────────────────
        if (info.contains("gear") && info["gear"].is_array()) {
            // First pass: ilvl range for gradient colour
            int minIlvl = INT_MAX, maxIlvl = 0;
            for (auto& item : info["gear"]) {
                if (item.value("id", 0) == 0) continue;
                int il = item.value("itemLevel", 0);
                if (il <= 1) continue;
                if (il < minIlvl) minIlvl = il;
                if (il > maxIlvl) maxIlvl = il;
            }
            bool allSame = (maxIlvl == 0) || (minIlvl == maxIlvl);

            struct GradStop { float t, r, g, b; };
            static constexpr GradStop kStops[] = {
                { 0.00f, 1.0f, 1.0f,  1.0f  },
                { 0.25f, 0.1f, 1.0f,  0.1f  },
                { 0.50f, 0.3f, 0.5f,  1.0f  },
                { 0.75f, 0.7f, 0.3f,  1.0f  },
                { 1.00f, 1.0f, 0.85f, 0.0f  },
            };

            auto ilvlColor = [&](int il) -> ImVec4 {
                if (il <= 1 || maxIlvl == 0)
                    return ImVec4(0.55f, 0.55f, 0.55f, 1.0f);
                float t = allSame ? 1.0f
                    : std::clamp((float)(il - minIlvl) /
                                 (float)(maxIlvl - minIlvl), 0.0f, 1.0f);
                for (int i = 0; i < 4; ++i) {
                    if (t <= kStops[i + 1].t) {
                        float s = (t - kStops[i].t) /
                                  (kStops[i + 1].t - kStops[i].t);
                        return ImVec4(
                            kStops[i].r + s*(kStops[i+1].r - kStops[i].r),
                            kStops[i].g + s*(kStops[i+1].g - kStops[i].g),
                            kStops[i].b + s*(kStops[i+1].b - kStops[i].b),
                            1.0f);
                    }
                }
                return ImVec4(kStops[4].r, kStops[4].g, kStops[4].b, 1.0f);
            };

            for (auto& item : info["gear"]) {
                int s = item.value("slot", -1);
                if (s < 0 || s >= 18) continue;

                GearSlot& gs = d.gear[s];
                gs.slot  = s;
                gs.empty = (item.value("id", 0) == 0);

                if (!gs.empty) {
                    gs.name      = item.value("name", "Unknown");
                    gs.itemLevel = item.value("itemLevel", 0);
                    gs.isTier    = item.contains("setID");

                    ImVec4      tc;
                    std::string tname;
                    gs.hasTrack = trackColor(item, tc, tname);
                    if (gs.hasTrack) {
                        gs.trackColor = tc;
                        gs.trackName  = tname;
                    }

                    // Priority: tier > track > ilvl gradient
                    if (gs.isTier)
                        gs.color = ImVec4(1.0f, 0.2f, 0.2f, 1.0f);
                    else if (gs.hasTrack)
                        gs.color = tc;
                    else
                        gs.color = ilvlColor(gs.itemLevel);
                }
            }
        }

        // ── Spell cast timeline ───────────────────────────────────────────────
        const WoWclass* pwc = nullptr;
        for (auto& entry : kSpecMap) {
            if (d.playerClass == entry.cls && d.playerSpec == entry.spec) {
                pwc = entry.wc;
                break;
            }
        }

        d.hasSpellData = (pwc != nullptr);
        d.hasEventData = root.contains("events") &&
                         root["events"].is_array() &&
                         !root["events"].empty();

        if (d.hasSpellData && d.hasEventData) {
            auto makeSet = [](const std::vector<int>& v) {
                return std::unordered_set<int>(v.begin(), v.end());
            };
            const auto rotSet  = makeSet(pwc->getRotSpells());
            const auto offSet  = makeSet(pwc->getOffSpells());
            const auto defSet  = makeSet(pwc->getDefSpells());
            const auto mobSet  = makeSet(pwc->getMobSpells());
            const auto utilSet = makeSet(pwc->getUtilSpells());

            int    unknownCnt = 0;
            double t0         = -1.0;

            for (const auto& ev : root["events"]) {
                if (!ev.contains("sourceID")) continue;
                if (ev["sourceID"].get<int>() != playerId) continue;
                if (ev.value("melee", false)) continue;
                if (ev.value("fake",  false)) continue;

                const int    sid = ev.value("abilityGameID", 0);
                const double ts  = ev.value("timestamp", 0.0);
                if (sid == 0 || sid == 1) continue;

                if (t0 < 0.0) t0 = ts;
                const double t = (ts - t0) / 1000.0;
                d.tMax = std::max(d.tMax, t);

                if      (rotSet.count(sid))  d.rotTs.push_back(t);
                else if (offSet.count(sid))  d.offTs.push_back(t);
                else if (defSet.count(sid))  d.defTs.push_back(t);
                else if (mobSet.count(sid))  d.mobTs.push_back(t);
                else if (utilSet.count(sid)) d.utilTs.push_back(t);
                else                         ++unknownCnt;
            }
            d.unknownCount = unknownCnt;

            // ── Boss cast timeline ────────────────────────────────────────────
            try {
                json bj = json::parse(bossJson);
                std::vector<std::string>                  bossOrder;
                std::unordered_map<std::string, BossRow>  bossMap;

                if (bj.contains("bosses")) {
                    for (const auto& b : bj["bosses"]) {
                        std::string name = b.value("name", "Unknown");
                        if (bossMap.find(name) == bossMap.end()) {
                            bossOrder.push_back(name);
                            bossMap[name].name = name;
                        }
                    }
                }
                if (bj.contains("castTimeline")) {
                    for (const auto& ev : bj["castTimeline"]) {
                        std::string name = ev.value("bossName", "Unknown");
                        double ts = ev.value("timestamp", 0.0);
                        double t  = (t0 >= 0.0) ? (ts - t0) / 1000.0
                                                 : ts / 1000.0;
                        d.tMax = std::max(d.tMax, t);
                        if (bossMap.find(name) != bossMap.end())
                            bossMap[name].ts.push_back(t);
                    }
                }
                for (const auto& name : bossOrder)
                    d.bossRows.push_back(std::move(bossMap[name]));

            } catch (const std::exception& e) {
                std::cerr << "[Timeline] boss JSON parse error: "
                          << e.what() << "\n";
            }
        }

        d.parseOk = true;

    } catch (const json::exception& e) {
        d.parseOk    = false;
        d.parseError = e.what();
    }

    return d;
}

// ── RenderPlayerWindow: pure display — reads g_playerData, writes nothing ─────
static void RenderPlayerWindow(ImGuiIO& io,
                               ImPlotColormap g_secondariesColormap)
{
    std::lock_guard<std::mutex> dlock(g_debugMutex);

    ImGui::SetNextWindowSize(ImVec2(720, 600), ImGuiCond_Appearing);
    ImGui::SetNextWindowPos(
        ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f),
        ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    ImGui::Begin(g_debugWindowTitle.c_str(), &g_debugWindowOpen);

    if (g_debugFetching.load()) {
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f),
                           "Fetching from server...");
        ImGui::End();
        return;
    }

    if (!g_playerDataReady) {
        ImGui::TextDisabled("No data loaded yet.");
        ImGui::End();
        return;
    }

    const PlayerWindowData& d = g_playerData;

    if (!d.parseOk) {
        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f),
                           "JSON parse error: %s", d.parseError.c_str());
        ImGui::End();
        return;
    }

    // ── Name / Class / Spec banner ────────────────────────────────────────────
    ImGui::Separator();
    ImGui::TextColored(ImVec4(0.3f, 0.9f, 0.5f, 1.0f),
                       "%s", d.playerName.c_str());
    ImGui::SameLine();
    ImGui::TextDisabled("(%s %s  |  Role: %s)",
                        d.playerSpec.c_str(),
                        d.playerClass.c_str(),
                        d.playerRole.c_str());
    ImGui::Separator();

    // ── Item level ────────────────────────────────────────────────────────────
    ImGui::Text("Item Level");
    ImGui::SameLine(160);
    ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.0f, 1.0f), "%.1f", d.ilvl);
    ImGui::Spacing();
    ImGui::Separator();

    // Helper: print a single stat row
    auto printStat = [](const char* label, int val, ImVec4 col) {
        if (val == 0) return;
        ImGui::Text("  %s", label);
        ImGui::SameLine(160);
        ImGui::TextColored(col, "%d", val);
    };
    static const ImVec4 kGold = ImVec4(1.0f, 0.75f, 0.2f, 1.0f);

    // ── Primary stats ─────────────────────────────────────────────────────────
    ImGui::TextColored(ImVec4(0.7f, 0.7f, 1.0f, 1.0f), "Primary Stats");
    ImGui::Separator();
    printStat("Agility",   d.agility,   kGold);
    printStat("Strength",  d.strength,  kGold);
    printStat("Intellect", d.intellect, kGold);
    printStat("Stamina",   d.stamina,   kGold);
    ImGui::Spacing();

    // ── Secondary stats ───────────────────────────────────────────────────────
    ImGui::TextColored(ImVec4(0.7f, 0.7f, 1.0f, 1.0f), "Secondary Stats");
    ImGui::Separator();
    printStat("Crit",        d.crit,        ImVec4(1.0f, 0.4f, 0.4f, 1.0f));
    printStat("Haste",       d.haste,       ImVec4(0.4f, 1.0f, 0.4f, 1.0f));
    printStat("Mastery",     d.mastery,     ImVec4(0.4f, 0.8f, 1.0f, 1.0f));
    printStat("Versatility", d.versatility, ImVec4(0.9f, 0.6f, 1.0f, 1.0f));
    ImGui::Spacing();

    // ── Pie chart ─────────────────────────────────────────────────────────────
    {
        static const char* kPieLabels[] = { "##0", "##1", "##2", "##3" };
        ImPlot::PushColormap(g_secondariesColormap);
        if (ImPlot::BeginPlot("##PieChart", ImVec2(250, 250),
                              ImPlotFlags_Equal     |
                              ImPlotFlags_NoMouseText |
                              ImPlotFlags_NoInputs  |
                              ImPlotFlags_NoLegend))
        {
            ImPlot::SetupAxes(nullptr, nullptr,
                              ImPlotAxisFlags_NoDecorations,
                              ImPlotAxisFlags_NoDecorations);
            ImPlot::PlotPieChart(kPieLabels, d.pieValues, 4,
                                 0.5, 0.5, 0.4, "%.0f");
            ImPlot::EndPlot();
        }
        ImPlot::PopColormap();
    }
    ImGui::Spacing();

    // ── Defensive stats ───────────────────────────────────────────────────────
    ImGui::TextColored(ImVec4(0.7f, 0.7f, 1.0f, 1.0f), "Defensive Stats");
    ImGui::Separator();
    printStat("Armor",     d.armor,     ImVec4(0.8f, 0.8f, 0.8f, 1.0f));
    printStat("Leech",     d.leech,     ImVec4(0.6f, 1.0f, 0.6f, 1.0f));
    printStat("Avoidance", d.avoidance, ImVec4(0.8f, 0.8f, 0.8f, 1.0f));
    printStat("Speed",     d.speed,     ImVec4(0.8f, 0.8f, 0.8f, 1.0f));
    ImGui::Spacing();
    ImGui::Separator();

    // ── Equipped gear ─────────────────────────────────────────────────────────
    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.7f, 0.7f, 1.0f, 1.0f), "Equipped Gear");
    ImGui::Separator();

    static const char* kSlotLabel[18] = {
        "Head",      "Neck",      "Shoulders", "Shirt",
        "Chest",     "Waist",     "Legs",      "Feet",
        "Wrists",    "Hands",     "Ring 1",    "Ring 2",
        "Trinket 1", "Trinket 2", "Back",      "Main Hand",
        "Off Hand",  "Ranged"
    };

    auto renderSlot = [&](int slot) {
        if (slot < 0 || slot >= 18) return;
        const char*     label = kSlotLabel[slot];
        const GearSlot& gs    = d.gear[slot];

        if (gs.empty || gs.slot == -1) {
            ImGui::TextDisabled("  %-11s  —", label);
            return;
        }

        ImGui::TextDisabled("  %-11s", label);
        ImGui::SameLine();
        ImGui::TextColored(gs.color, "%s - %d",
                           gs.name.c_str(), gs.itemLevel);

        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            if (gs.isTier) {
                ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f),
                                   "Tier / Set Piece");
                if (gs.hasTrack)
                    ImGui::TextColored(gs.trackColor,
                                       "Upgrade: %s", gs.trackName.c_str());
            } else if (gs.hasTrack) {
                ImGui::TextColored(gs.trackColor,
                                   "Track: %s", gs.trackName.c_str());
            } else {
                ImGui::TextDisabled("Track: Unknown (ilvl fallback)");
            }
            ImGui::EndTooltip();
        }
    };

    static const int kLeftSlots[]  = { 0, 1, 2, 14, 4, 3, 8 };
    static const int kRightSlots[] = { 9, 5, 6, 7, 10, 11, 12, 13 };

    ImGui::Columns(2, "##gearCols", false);
    for (int s : kLeftSlots)  renderSlot(s);
    ImGui::NextColumn();
    for (int s : kRightSlots) renderSlot(s);
    ImGui::Columns(1);

    ImGui::Separator();
    renderSlot(15);  // Main Hand
    renderSlot(16);  // Off Hand

    // ── Spell cast timelines ──────────────────────────────────────────────────
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.7f, 0.7f, 1.0f, 1.0f), "Spell Cast Timelines");
    ImGui::Separator();
    ImGui::Spacing();

    if (!d.hasSpellData) {
        ImGui::TextDisabled(
            "No hardcoded spell data found for %s %s — timeline unavailable.",
            d.playerSpec.c_str(), d.playerClass.c_str());
    } else if (!d.hasEventData) {
        ImGui::TextDisabled("No cast events available for timeline.");
    } else {
        const int kPlayerRows = 5;
        const int kGap        = 1;
        const int kBossBase   = kPlayerRows + kGap;
        const int totalRows   = kBossBase + (int)d.bossRows.size();

        const double yMin = -0.6;
        const double yMax = (double)totalRows - 0.4;

        // Build Y-axis tick data (stack-local; outlives the plot call below)
        std::vector<double>      yTickVals;
        std::vector<const char*> yTickLabels;
        std::vector<std::string> bossLabelStorage;

        static const double     kPlayerY[]      = { 0,1,2,3,4 };
        static const char*      kPlayerLabels[] = {
            "Utility","Mobility","Defensive","Offensive","Rotation"
        };
        for (int i = 0; i < kPlayerRows; ++i) {
            yTickVals.push_back(kPlayerY[i]);
            yTickLabels.push_back(kPlayerLabels[i]);
        }
        yTickVals.push_back((double)kPlayerRows);
        yTickLabels.push_back("");

        bossLabelStorage.reserve(d.bossRows.size());
        for (int i = 0; i < (int)d.bossRows.size(); ++i) {
            bossLabelStorage.push_back(d.bossRows[i].name);
            yTickVals.push_back((double)(kBossBase + i));
            yTickLabels.push_back(bossLabelStorage.back().c_str());
        }

        const float plotHeight = 28.0f * (float)totalRows;

        if (ImPlot::BeginPlot("##CastTimeline",
                              ImVec2(-1, plotHeight),
                              ImPlotFlags_NoMouseText |
                              ImPlotFlags_NoLegend))
        {
            ImPlot::SetupAxes("Time (s)", nullptr,
                              ImPlotAxisFlags_None,
                              ImPlotAxisFlags_NoGridLines |
                              ImPlotAxisFlags_NoTickMarks);
            ImPlot::SetupAxisTicks(ImAxis_Y1,
                                   yTickVals.data(),
                                   (int)yTickVals.size(),
                                   yTickLabels.data());
            ImPlot::SetupAxisLimits(ImAxis_X1, 0.0,
                                    d.tMax > 0.0 ? d.tMax * 1.05 : 60.0,
                                    ImGuiCond_Always);
            ImPlot::SetupAxisLimits(ImAxis_Y1, yMin, yMax,
                                    ImGuiCond_Always);

            auto plotRow = [](const std::vector<double>& ts,
                              double rowY, const char* id) {
                if (ts.empty()) return;
                std::vector<double> ys(ts.size(), rowY);
                ImPlot::PlotScatter(id, ts.data(), ys.data(),
                                    (int)ts.size());
            };

            plotRow(d.rotTs,  4.0, "##rot");
            plotRow(d.offTs,  3.0, "##off");
            plotRow(d.defTs,  2.0, "##def");
            plotRow(d.mobTs,  1.0, "##mob");
            plotRow(d.utilTs, 0.0, "##util");

            for (int i = 0; i < (int)d.bossRows.size(); ++i) {
                std::string id = "##boss" + std::to_string(i);
                plotRow(d.bossRows[i].ts,
                        (double)(kBossBase + i),
                        id.c_str());
            }

            ImPlot::EndPlot();
        }

        // ── Cast-count summary ────────────────────────────────────────────────
        ImGui::Spacing();
        ImGui::Columns(5, "##castCounts", true);

        struct CatSummary {
            const char*              name;
            const std::vector<double>* ts;
            ImVec4                   col;
        };
        const CatSummary kSummary[] = {
            { "Rotation",  &d.rotTs,  ImVec4(1.00f,0.35f,0.35f,1.0f) },
            { "Offensive", &d.offTs,  ImVec4(1.00f,0.70f,0.10f,1.0f) },
            { "Defensive", &d.defTs,  ImVec4(0.30f,0.70f,1.00f,1.0f) },
            { "Mobility",  &d.mobTs,  ImVec4(0.40f,1.00f,0.40f,1.0f) },
            { "Utility",   &d.utilTs, ImVec4(0.90f,0.60f,1.00f,1.0f) },
        };
        for (auto& s : kSummary) {
            ImGui::TextColored(s.col, "%s", s.name);
            ImGui::Text("%d casts", (int)s.ts->size());
            ImGui::NextColumn();
        }
        ImGui::Columns(1);

        if (d.unknownCount > 0) {
            ImGui::Spacing();
            ImGui::TextDisabled(
                "(%d casts not matched to any spell list — check specData.h)",
                d.unknownCount);
        }
    }

    ImGui::End();
}

int main() {
    static std::string       tokenResponse;
    static bool              tokenFetched = false;
    static char              wclUrlBuf[512] = "";
    static std::atomic<bool> isFetching(false);
    static std::string       fetchStatus;
    std::vector<std::string> keys;
    std::string              errorMsg;

    // ── Token helper (checks file cache, then falls back to OAuth) ────────────
    auto FetchToken = [&]() {
        std::ifstream tokenFile(KEY_FILE_DIRECTORY);
        if (tokenFile.is_open()) {
            std::string line;
            while (std::getline(tokenFile, line)) {
                if (line.rfind("token=", 0) == 0) {
                    std::string existing = line.substr(6);
                    if (!existing.empty()) {
                        tokenResponse = existing;
                        tokenFetched  = true;
                        return;
                    }
                }
            }
        }

        std::string raw = FetchOAuthToken(keys[0], keys[1]);
        try {
            auto j = json::parse(raw);
            tokenResponse = j.contains("access_token")
                              ? j["access_token"].get<std::string>()
                              : raw;
        } catch (...) {
            tokenResponse = "JSON parse error: " + raw;
        }

        if (!tokenResponse.empty() && tokenResponse[0] != '{') {
            appendToken(KEY_FILE_DIRECTORY, tokenResponse);
            tokenFetched = true;
        }
    };

    try {
        keys = fetchKeys(KEY_FILE_DIRECTORY);
    } catch (const std::exception& e) {
        errorMsg = e.what();
    }

    // ── GLFW / ImGui initialisation ───────────────────────────────────────────
    if (!glfwInit()) return -1;

    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    GLFWwindow* window =
        glfwCreateWindow(960, 400, "Beard Tools", nullptr, nullptr);
    if (!window) { glfwTerminate(); return -1; }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    static const ImVec4 s_pieColors[] = {
        ImVec4(1.0f, 0.4f, 0.4f, 1.0f),
        ImVec4(0.4f, 1.0f, 0.4f, 1.0f),
        ImVec4(0.4f, 0.8f, 1.0f, 1.0f),
        ImVec4(0.9f, 0.6f, 1.0f, 1.0f),
    };
    ImPlotColormap g_secondariesColormap =
        ImPlot::AddColormap("secondaries color", s_pieColors, 4);

    FetchToken();

    // ── Main render loop ──────────────────────────────────────────────────────
    while (!glfwWindowShouldClose(window)) {
        bool currentlyFetching = isFetching.load();
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGuiIO& io = ImGui::GetIO();
        ImGui::SetNextWindowPos(
            ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f),
            ImGuiCond_Always, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(880, 320), ImGuiCond_Always);

        ImGui::Begin("Main Window", nullptr,
            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse);

        if (!errorMsg.empty()) {
            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f),
                               "Error: %s", errorMsg.c_str());
        } else {
            ImGui::Text("Loaded key from: %s", KEY_FILE_DIRECTORY.c_str());
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // ── WCL URL input ─────────────────────────────────────────────────────
        ImGui::Text("WCL Report URL:");
        ImGui::SetNextItemWidth(700.0f);
        ImGui::InputText("##wclurl", wclUrlBuf, sizeof(wclUrlBuf));
        ImGui::SameLine();

        if (currentlyFetching) ImGui::BeginDisabled();

        if (ImGui::Button("Fetch Log")) {
            std::string urlCopy      = wclUrlBuf;
            std::string clientId     = keys[0];
            std::string clientSecret = keys[1];

            isFetching  = true;
            fetchStatus = "Fetching...";

            std::thread([urlCopy, clientId, clientSecret]() {
                std::string reportCode = ExtractReportCode(urlCopy);
                std::string tokenJson  = FetchOAuthToken(clientId, clientSecret);
                std::string reportJson = FetchReportData(tokenJson, reportCode);

                try {
                    auto root = json::parse(reportJson);

                    if (!root.contains("data") || root["data"].is_null() ||
                        !root["data"].contains("reportData") ||
                        root["data"]["reportData"].is_null() ||
                        !root["data"]["reportData"].contains("report") ||
                        root["data"]["reportData"]["report"].is_null()) {
                        fetchStatus = "API error: " + reportJson;
                        std::cout << "API error: " + reportJson;
                        isFetching = false;
                        return;
                    }

                    auto& rep = root["data"]["reportData"]["report"];

                    ReportData parsed;
                    parsed.title = (rep.contains("title") && rep["title"].is_string())
                                     ? rep["title"].get<std::string>() : "";
                    parsed.owner = (rep.contains("owner") &&
                                    rep["owner"].is_object() &&
                                    rep["owner"].contains("name"))
                                     ? rep["owner"]["name"].get<std::string>() : "";

                    struct PlayerInfo { std::string name; std::string role; };
                    std::unordered_map<int, PlayerInfo> playerLookup;

                    if (rep.contains("playerDetails") &&
                        !rep["playerDetails"].is_null()) {
                        json pd = rep["playerDetails"].is_string()
                                    ? json::parse(rep["playerDetails"].get<std::string>())
                                    : rep["playerDetails"];

                        if (pd.contains("data") && pd["data"].is_object() &&
                            pd["data"].contains("playerDetails") &&
                            pd["data"]["playerDetails"].is_object()) {

                            auto& pdInner = pd["data"]["playerDetails"];
                            for (const char* role : {"tanks", "healers", "dps"}) {
                                if (!pdInner.contains(role) ||
                                    !pdInner[role].is_array()) continue;
                                for (auto& p : pdInner[role]) {
                                    if (!p.is_object() ||
                                        !p.contains("id") ||
                                        !p["id"].is_number()) continue;
                                    int pid = p["id"].get<int>();
                                    std::string pname =
                                        (p.contains("name") && p["name"].is_string())
                                          ? p["name"].get<std::string>() : "?";
                                    playerLookup[pid] = { pname, role };
                                }
                            }
                        }
                    }

                    if (!rep.contains("fights") || !rep["fights"].is_array()) {
                        fetchStatus = "No fights data in response";
                        isFetching  = false;
                        return;
                    }

                    for (auto& f : rep["fights"]) {
                        if (!f.is_object()) continue;
                        if (!f.contains("kill")       || f["kill"].is_null())       continue;
                        if (!f.contains("difficulty") || f["difficulty"].is_null()) continue;

                        Fight fight;
                        fight.id        = (f.contains("id") && f["id"].is_number())
                                          ? f["id"].get<int>()           : 0;
                        fight.name      = (f.contains("name") && f["name"].is_string())
                                          ? f["name"].get<std::string>() : "";
                        fight.startTime = (f.contains("startTime") && f["startTime"].is_number())
                                          ? f["startTime"].get<double>() : 0.0;
                        fight.endTime   = (f.contains("endTime") && f["endTime"].is_number())
                                          ? f["endTime"].get<double>()   : 0.0;
                        fight.percent   = (f.contains("bossPercentage") &&
                                           f["bossPercentage"].is_number())
                                          ? f["bossPercentage"].get<double>() : 0.0;
                        fight.kill       = f["kill"].get<bool>();
                        fight.difficulty = f["difficulty"].get<int>();

                        if (f.contains("friendlyPlayers") &&
                            f["friendlyPlayers"].is_array()) {
                            for (auto& pidVal : f["friendlyPlayers"]) {
                                if (!pidVal.is_number()) continue;
                                int pid = pidVal.get<int>();
                                auto it = playerLookup.find(pid);
                                if (it == playerLookup.end()) continue;

                                const auto& info = it->second;
                                PlayerEntry entry{ pid, info.name };
                                if      (info.role == "tanks")   fight.tanks.push_back(entry);
                                else if (info.role == "healers") fight.healers.push_back(entry);
                                else if (info.role == "dps")     fight.dps.push_back(entry);
                            }
                        }

                        parsed.fights.push_back(std::move(fight));
                    }

                    {
                        std::lock_guard<std::mutex> lock(g_reportMutex);
                        g_report            = std::move(parsed);
                        g_cachedReportCode  = reportCode;
                        g_cachedBearerToken = tokenJson;
                    }
                    g_reportReady = true;
                    fetchStatus   = "Done";
                } catch (const std::exception& e) {
                    fetchStatus = std::string("Parse error: ") + e.what();
                }

                isFetching = false;
            }).detach();
        }

        if (currentlyFetching) ImGui::EndDisabled();

        if (!fetchStatus.empty()) {
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(0.6f, 0.9f, 0.6f, 1.0f),
                               "%s", fetchStatus.c_str());
        }

        // ── Fight list ────────────────────────────────────────────────────────
        if (g_reportReady.load()) {
            std::lock_guard<std::mutex> lock(g_reportMutex);

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            ImGui::Text("Report: %s  (owner: %s)",
                        g_report.title.c_str(), g_report.owner.c_str());
            ImGui::Spacing();

            auto diffLabel = [](std::optional<int> d) -> const char* {
                if (!d) return "—";
                switch (*d) {
                    case 5: return "Mythic";
                    case 4: return "Heroic";
                    case 3: return "Normal";
                    case 2: return "LFR";
                    case 1: return "Story";
                    default: return "?";
                }
            };

            auto headerColor = [](std::optional<bool> kill) -> ImVec4 {
                if (!kill) return ImVec4(0.35f, 0.35f, 0.35f, 1.0f);
                if (*kill) return ImVec4(0.10f, 0.45f, 0.10f, 1.0f);
                return            ImVec4(0.50f, 0.10f, 0.10f, 1.0f);
            };
            auto headerColorHov = [](std::optional<bool> kill) -> ImVec4 {
                if (!kill) return ImVec4(0.45f, 0.45f, 0.45f, 1.0f);
                if (*kill) return ImVec4(0.15f, 0.60f, 0.15f, 1.0f);
                return            ImVec4(0.65f, 0.15f, 0.15f, 1.0f);
            };
            auto headerColorAct = [](std::optional<bool> kill) -> ImVec4 {
                if (!kill) return ImVec4(0.55f, 0.55f, 0.55f, 1.0f);
                if (*kill) return ImVec4(0.20f, 0.70f, 0.20f, 1.0f);
                return            ImVec4(0.75f, 0.20f, 0.20f, 1.0f);
            };

            auto renderPlayerButtons = [&](
                const std::vector<PlayerEntry>& players,
                const Fight&                    fight,
                const char*                     roleTag,
                int                             wrapAfter)
            {
                bool fetching  = g_debugFetching.load();
                g_currentFight = fight;

                for (int i = 0; i < (int)players.size(); ++i) {
                    const PlayerEntry& pe = players[i];

                    std::string btnId = pe.name
                                      + "##" + roleTag
                                      + "_f"  + std::to_string(fight.id)
                                      + "_p"  + std::to_string(pe.id);

                    bool wasDisabled = fetching;
                    if (wasDisabled) ImGui::BeginDisabled();

                    if (ImGui::Button(btnId.c_str())) {
                        Fight       capturedFight  = fight;
                        std::string capturedName   = pe.name;
                        std::string capturedFightN = fight.name;
                        int         capturedPid    = pe.id;
                        std::string capturedCode   = g_cachedReportCode;
                        std::string capturedToken  = g_cachedBearerToken;

                        {
                            std::lock_guard<std::mutex> dlock(g_debugMutex);
                            g_debugWindowTitle = capturedName
                                              + " \xe2\x80\x94 " + capturedFightN;
                            g_debugRawResponse = "Fetching\xe2\x80\xa6";
                            g_debugWindowOpen  = true;
                            g_debugPlayerId    = capturedPid;
                        }

                        // Mark that we need to (re)build the cached data
                        // once the background thread finishes.
                        g_playerDataReady = false;
                        g_wasFetching     = true;
                        g_debugFetching   = true;
                        fetching          = true;

                        std::thread([capturedFight, capturedPid,
                                     capturedCode, capturedToken]() {
                            std::string raw = fetchPersonalFightData(
                                capturedFight, capturedPid,
                                capturedCode, capturedToken);
                            {
                                std::lock_guard<std::mutex> dlock(g_debugMutex);
                                g_debugRawResponse = raw;
                            }

                            if (!g_fetchedBossData) {
                                std::string bossJson = fetchBossCastTimeline(
                                    capturedFight, capturedCode, capturedToken);
                                {
                                    std::lock_guard<std::mutex> dlock(g_debugMutex);
                                    g_cachedBossJson = bossJson;
                                }
                                g_fetchedBossData = true;
                            }

                            g_debugFetching = false;
                        }).detach();
                    }

                    if (wasDisabled) ImGui::EndDisabled();

                    if (wrapAfter > 0 &&
                        (i + 1) % wrapAfter != 0 &&
                        i + 1 < (int)players.size())
                        ImGui::SameLine();
                }
            };

            ImGui::BeginChild("##fights", ImVec2(0, 0), false,
                              ImGuiWindowFlags_HorizontalScrollbar);

            for (auto& fight : g_report.fights) {
                double durationSec = (fight.endTime - fight.startTime) / 1000.0;
                int    mins        = static_cast<int>(durationSec) / 60;
                int    secs        = static_cast<int>(durationSec) % 60;

                const char* killStr = fight.kill
                                        ? (*fight.kill ? "Kill" : "Wipe") : "—";
                char label[256];
                char percentStr[16];
                snprintf(percentStr, sizeof(percentStr), "%.2f%%", fight.percent);
                snprintf(label, sizeof(label),
                         "[%d] %s  |  %s  |  %d:%02d  |  %s | %s",
                         fight.id, fight.name.c_str(),
                         diffLabel(fight.difficulty),
                         mins, secs, killStr, percentStr);

                ImGui::PushStyleColor(ImGuiCol_Header,
                                      headerColor(fight.kill));
                ImGui::PushStyleColor(ImGuiCol_HeaderHovered,
                                      headerColorHov(fight.kill));
                ImGui::PushStyleColor(ImGuiCol_HeaderActive,
                                      headerColorAct(fight.kill));

                if (ImGui::CollapsingHeader(label)) {
                    ImGui::Indent(16.0f);

                    if (!fight.tanks.empty()) {
                        ImGui::TextColored(ImVec4(0.3f, 0.6f, 1.0f, 1.0f), "Tanks");
                        ImGui::Spacing();
                        renderPlayerButtons(fight.tanks, fight, "tank", 0);
                        ImGui::NewLine();
                        ImGui::Spacing();
                    }
                    if (!fight.healers.empty()) {
                        ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.5f, 1.0f), "Healers");
                        ImGui::Spacing();
                        renderPlayerButtons(fight.healers, fight, "heal", 5);
                        ImGui::NewLine();
                        ImGui::Spacing();
                    }
                    if (!fight.dps.empty()) {
                        ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "DPS");
                        ImGui::Spacing();
                        renderPlayerButtons(fight.dps, fight, "dps", 5);
                        ImGui::NewLine();
                        ImGui::Spacing();
                    }
                    if (fight.tanks.empty() &&
                        fight.healers.empty() &&
                        fight.dps.empty())
                        ImGui::TextDisabled("No player data available.");

                    ImGui::Unindent(16.0f);
                    ImGui::Spacing();
                }

                ImGui::PopStyleColor(3);
            }

            ImGui::EndChild();
        }

        ImGui::End();

        // ── Edge-detect: g_debugFetching just went false → parse ONCE ─────────
        //
        // g_wasFetching is set to true when a player button is clicked.
        // The background thread sets g_debugFetching = false when done.
        // We catch that falling edge here on the main thread, outside any
        // ImGui Begin/End pair, so it runs exactly once per player request.
        if (g_wasFetching && !g_debugFetching.load()) {
            g_wasFetching = false;

            // Snapshot the shared strings under the mutex, then parse off it.
            std::string rawSnap, bossSnap;
            int         pidSnap;
            {
                std::lock_guard<std::mutex> dlock(g_debugMutex);
                rawSnap  = g_debugRawResponse;
                bossSnap = g_cachedBossJson;
                pidSnap  = g_debugPlayerId;
            }

            g_playerData      = BuildPlayerData(rawSnap, pidSnap, bossSnap);
            g_playerDataReady = true;
        }

        // ── Per-player detail window ──────────────────────────────────────────
        if (g_debugWindowOpen)
            RenderPlayerWindow(io, g_secondariesColormap);

        // ── Render ────────────────────────────────────────────────────────────
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    // ── Cleanup ───────────────────────────────────────────────────────────────
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}