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

    GLFWwindow* window = glfwCreateWindow(960, 400, "Beard Tools", nullptr, nullptr);
    if (!window) { glfwTerminate(); return -1; }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Pie-chart colormap for secondary stats
    static const ImVec4 s_pieColors[] = {
        ImVec4(1.0f, 0.4f, 0.4f, 1.0f),   // Crit
        ImVec4(0.4f, 1.0f, 0.4f, 1.0f),   // Haste
        ImVec4(0.4f, 0.8f, 1.0f, 1.0f),   // Mastery
        ImVec4(0.9f, 0.6f, 1.0f, 1.0f),   // Versatility
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

                    // ── Build id -> {name, role} lookup from playerDetails ──────
                    struct PlayerInfo { std::string name; std::string role; };
                    std::unordered_map<int, PlayerInfo> playerLookup;

                    if (rep.contains("playerDetails") && !rep["playerDetails"].is_null()) {
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

                    // ── Parse fights ──────────────────────────────────────────────
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
                        fight.id        = (f.contains("id")        && f["id"].is_number())
                                          ? f["id"].get<int>()           : 0;
                        fight.name      = (f.contains("name")      && f["name"].is_string())
                                          ? f["name"].get<std::string>() : "";
                        fight.startTime = (f.contains("startTime") && f["startTime"].is_number())
                                          ? f["startTime"].get<double>() : 0.0;
                        fight.endTime   = (f.contains("endTime")   && f["endTime"].is_number())
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

            // Renders a row of player buttons; fires a background fetch on click.
            auto renderPlayerButtons = [&](
                const std::vector<PlayerEntry>& players,
                const Fight&                    fight,
                const char*                     roleTag,
                int                             wrapAfter)
            {
                bool fetching = g_debugFetching.load();
                g_fetchedBossData = false;
                g_currentFight    = fight;

                for (int i = 0; i < (int)players.size(); ++i) {
                    const PlayerEntry& pe = players[i];

                    std::string btnId = pe.name
                                      + "##" + roleTag
                                      + "_f"  + std::to_string(fight.id)
                                      + "_p"  + std::to_string(pe.id);

                    bool wasDisabled = fetching;
                    if (wasDisabled) ImGui::BeginDisabled();

                    if (ImGui::Button(btnId.c_str())) {
                        Fight       capturedFight = fight;
                        std::string capturedName  = pe.name;
                        std::string capturedFightN= fight.name;
                        int         capturedPid   = pe.id;
                        std::string capturedCode  = g_cachedReportCode;
                        std::string capturedToken = g_cachedBearerToken;

                        {
                            std::lock_guard<std::mutex> dlock(g_debugMutex);
                            g_debugWindowTitle = capturedName + " \xe2\x80\x94 " + capturedFightN;
                            g_debugRawResponse = "Fetching\xe2\x80\xa6";
                            g_debugWindowOpen  = true;
                            g_debugPlayerId    = capturedPid;
                        }
                        g_debugFetching = true;
                        fetching        = true;

                        std::thread([capturedFight, capturedPid,
                                     capturedCode, capturedToken]() {
                            std::string raw = fetchPersonalFightData(
                                capturedFight, capturedPid, capturedCode, capturedToken);
                            {
                                std::lock_guard<std::mutex> dlock(g_debugMutex);
                                g_debugRawResponse = raw;
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

                ImGui::PushStyleColor(ImGuiCol_Header,        headerColor(fight.kill));
                ImGui::PushStyleColor(ImGuiCol_HeaderHovered, headerColorHov(fight.kill));
                ImGui::PushStyleColor(ImGuiCol_HeaderActive,  headerColorAct(fight.kill));

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
                    if (fight.tanks.empty() && fight.healers.empty() && fight.dps.empty())
                        ImGui::TextDisabled("No player data available.");

                    ImGui::Unindent(16.0f);
                    ImGui::Spacing();
                }

                ImGui::PopStyleColor(3);
            }

            ImGui::EndChild();
        }

        ImGui::End();

        // ── Per-player detail window ──────────────────────────────────────────
        if (g_debugWindowOpen) {
            std::lock_guard<std::mutex> dlock(g_debugMutex);

            ImGui::SetNextWindowSize(ImVec2(720, 600), ImGuiCond_Appearing);
            ImGui::SetNextWindowPos(
                ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f),
                ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

            ImGui::Begin(g_debugWindowTitle.c_str(), &g_debugWindowOpen);

            if (g_debugFetching.load()) {
                ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f),
                                   "Fetching from server...");
            } else {
                try {
                    auto root       = json::parse(g_debugRawResponse);
                    auto& tableData = root["table"]["data"];
                    auto& info      = tableData["combatantInfo"];
                    auto& stats     = info["stats"];

                    // ── Resolve player identity from composition ───────────────
                    std::string playerName  = "Unknown";
                    std::string playerClass = "Unknown";
                    std::string playerSpec  = "Unknown";
                    std::string playerRole  = "Unknown";

                    if (tableData.contains("composition") &&
                        tableData["composition"].is_array()) {
                        for (auto& member : tableData["composition"]) {
                            if (!member.contains("id") ||
                                !member["id"].is_number()) continue;
                            if (member["id"].get<int>() != g_debugPlayerId) continue;

                            playerName  = member.value("name", "Unknown");
                            playerClass = member.value("type", "Unknown");

                            if (member.contains("specs") &&
                                member["specs"].is_array() &&
                                !member["specs"].empty()) {
                                auto& spec = member["specs"][0];
                                playerSpec = spec.value("spec", "Unknown");
                                playerRole = spec.value("role", "Unknown");
                            }
                            break;
                        }
                    }

                    float ilvl = tableData.value("itemLevel", 0.0f);

                    // ── Name / Class / Spec banner ────────────────────────────
                    ImGui::Separator();
                    ImGui::TextColored(ImVec4(0.3f, 0.9f, 0.5f, 1.0f),
                                       "%s", playerName.c_str());
                    ImGui::SameLine();
                    ImGui::TextDisabled("(%s %s  |  Role: %s)",
                                        playerSpec.c_str(),
                                        playerClass.c_str(),
                                        playerRole.c_str());
                    ImGui::Separator();

                    // ── Item level ────────────────────────────────────────────
                    ImGui::Text("Item Level");
                    ImGui::SameLine(160);
                    ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.0f, 1.0f), "%.1f", ilvl);
                    ImGui::Spacing();
                    ImGui::Separator();

                    // ── Primary stats ─────────────────────────────────────────
                    auto printStat = [&](const char* label, const char* key,
                                         ImVec4 col = ImVec4(1, 1, 1, 1)) {
                        if (stats.contains(key) && stats[key].contains("max")) {
                            int val = stats[key]["max"].get<int>();
                            ImGui::Text("  %s", label);
                            ImGui::SameLine(160);
                            ImGui::TextColored(col, "%d", val);
                        }
                    };

                    ImGui::TextColored(ImVec4(0.7f, 0.7f, 1.0f, 1.0f), "Primary Stats");
                    ImGui::Separator();
                    printStat("Agility",   "Agility",   ImVec4(1.0f, 0.75f, 0.2f, 1.0f));
                    printStat("Strength",  "Strength",  ImVec4(1.0f, 0.75f, 0.2f, 1.0f));
                    printStat("Intellect", "Intellect", ImVec4(1.0f, 0.75f, 0.2f, 1.0f));
                    printStat("Stamina",   "Stamina",   ImVec4(1.0f, 0.75f, 0.2f, 1.0f));
                    ImGui::Spacing();

                    // ── Secondary stats ───────────────────────────────────────
                    ImGui::TextColored(ImVec4(0.7f, 0.7f, 1.0f, 1.0f), "Secondary Stats");
                    ImGui::Separator();
                    printStat("Crit",        "Crit",        ImVec4(1.0f, 0.4f, 0.4f, 1.0f));
                    printStat("Haste",       "Haste",       ImVec4(0.4f, 1.0f, 0.4f, 1.0f));
                    printStat("Mastery",     "Mastery",     ImVec4(0.4f, 0.8f, 1.0f, 1.0f));
                    printStat("Versatility", "Versatility", ImVec4(0.9f, 0.6f, 1.0f, 1.0f));
                    ImGui::Spacing();

                    // ── Secondary stats pie chart ─────────────────────────────
                    const char* labels[] = { "##0", "##1", "##2", "##3" };
                    const char* keys[]   = { "Crit", "Haste", "Mastery", "Versatility" };
                    double values[4]     = {};
                    for (int i = 0; i < 4; ++i) {
                        if (stats.contains(keys[i]) && stats[keys[i]].contains("max"))
                            values[i] = stats[keys[i]]["max"].get<int>();
                    }

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
                        ImPlot::PlotPieChart(labels, values, 4,
                                             0.5, 0.5, 0.4, "%.0f");
                        ImPlot::EndPlot();
                    }
                    ImPlot::PopColormap();
                    ImGui::Spacing();

                    // ── Defensive stats ───────────────────────────────────────
                    ImGui::TextColored(ImVec4(0.7f, 0.7f, 1.0f, 1.0f), "Defensive Stats");
                    ImGui::Separator();
                    printStat("Armor",     "Armor",     ImVec4(0.8f, 0.8f, 0.8f, 1.0f));
                    printStat("Leech",     "Leech",     ImVec4(0.6f, 1.0f, 0.6f, 1.0f));
                    printStat("Avoidance", "Avoidance", ImVec4(0.8f, 0.8f, 0.8f, 1.0f));
                    printStat("Speed",     "Speed",     ImVec4(0.8f, 0.8f, 0.8f, 1.0f));
                    ImGui::Spacing();
                    ImGui::Separator();

                    // ── Equipped gear ─────────────────────────────────────────
                    ImGui::Spacing();
                    ImGui::TextColored(ImVec4(0.7f, 0.7f, 1.0f, 1.0f), "Equipped Gear");
                    ImGui::Separator();

                    if (info.contains("gear") && info["gear"].is_array()) {
                        auto& gear = info["gear"];

                        // Resolve upgrade track colour for a single item.
                        auto trackColor = [&](const json& item, ImVec4& out) -> bool {
                            if (!item.contains("bonusIDs") ||
                                !item["bonusIDs"].is_array()) return false;
                            for (auto& bid : item["bonusIDs"]) {
                                if (!bid.is_number_integer()) continue;
                                int id = bid.get<int>();
                                for (auto& track : kUpgradeTracks) {
                                    if (track.bonusID == id) {
                                        out = track.color;
                                        return true;
                                    }
                                }
                            }
                            return false;
                        };

                        // ilvl gradient fallback
                        int minIlvl = INT_MAX, maxIlvl = 0;
                        for (auto& item : gear) {
                            if (item.value("id", 0) == 0) continue;
                            int ilvl = item.value("itemLevel", 0);
                            if (ilvl <= 1) continue;
                            if (ilvl < minIlvl) minIlvl = ilvl;
                            if (ilvl > maxIlvl) maxIlvl = ilvl;
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

                        auto ilvlColor = [&](int ilvl) -> ImVec4 {
                            if (ilvl <= 1 || maxIlvl == 0)
                                return ImVec4(0.55f, 0.55f, 0.55f, 1.0f);
                            float t = allSame ? 1.0f
                                : std::clamp((float)(ilvl - minIlvl) /
                                             (float)(maxIlvl - minIlvl),
                                             0.0f, 1.0f);
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

                        // Priority: tier red > upgrade track > ilvl gradient
                        auto resolveColor = [&](const json& item) -> ImVec4 {
                            if (item.value("id", 0) == 0)
                                return ImVec4(0.55f, 0.55f, 0.55f, 1.0f);
                            if (item.contains("setID"))
                                return ImVec4(1.0f, 0.2f, 0.2f, 1.0f);
                            ImVec4 tc;
                            if (trackColor(item, tc)) return tc;
                            return ilvlColor(item.value("itemLevel", 0));
                        };

                        static const char* kSlotLabel[18] = {
                            "Head",      "Neck",      "Shoulders", "Shirt",
                            "Chest",     "Waist",     "Legs",      "Feet",
                            "Wrists",    "Hands",     "Ring 1",    "Ring 2",
                            "Trinket 1", "Trinket 2", "Back",      "Main Hand",
                            "Off Hand",  "Ranged"
                        };

                        std::map<int, const json*> slotMap;
                        for (auto& item : gear) {
                            int s = item.value("slot", -1);
                            if (s >= 0 && s < 18) slotMap[s] = &item;
                        }

                        auto renderSlot = [&](int slot) {
                            const char* label =
                                (slot >= 0 && slot < 18) ? kSlotLabel[slot] : "?";
                            auto it = slotMap.find(slot);

                            if (it == slotMap.end() ||
                                it->second->value("id", 0) == 0) {
                                ImGui::TextDisabled("  %-11s  —", label);
                                return;
                            }

                            auto& item = *it->second;
                            auto  name = item.value("name", "Unknown");
                            int   ilvl = item.value("itemLevel", 0);

                            ImGui::TextDisabled("  %-11s", label);
                            ImGui::SameLine();
                            ImGui::TextColored(resolveColor(item),
                                               "%s - %d", name.c_str(), ilvl);

                            if (ImGui::IsItemHovered()) {
                                ImGui::BeginTooltip();
                                ImVec4 trackCol;
                                if (item.contains("setID")) {
                                    ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f),
                                                       "Tier / Set Piece");
                                    for (auto& track : kUpgradeTracks) {
                                        bool matched =
                                            item.contains("bonusIDs") &&
                                            item["bonusIDs"].end() !=
                                            std::find_if(
                                                item["bonusIDs"].begin(),
                                                item["bonusIDs"].end(),
                                                [&](const json& b) {
                                                    return b.is_number_integer() &&
                                                           b.get<int>() == track.bonusID;
                                                });
                                        if (matched) {
                                            ImGui::TextColored(track.color,
                                                "Upgrade: %s", track.name);
                                            break;
                                        }
                                    }
                                } else if (trackColor(item, trackCol)) {
                                    for (auto& track : kUpgradeTracks) {
                                        bool matched =
                                            item.contains("bonusIDs") &&
                                            item["bonusIDs"].end() !=
                                            std::find_if(
                                                item["bonusIDs"].begin(),
                                                item["bonusIDs"].end(),
                                                [&](const json& b) {
                                                    return b.is_number_integer() &&
                                                           b.get<int>() == track.bonusID;
                                                });
                                        if (matched) {
                                            ImGui::TextColored(trackCol,
                                                "Track: %s", track.name);
                                            break;
                                        }
                                    }
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
                    }

                    // ── Spell cast timelines ──────────────────────────────────
                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::Spacing();
                    ImGui::TextColored(ImVec4(0.7f, 0.7f, 1.0f, 1.0f),
                                       "Spell Cast Timelines");
                    ImGui::Separator();
                    ImGui::Spacing();

                    const WoWclass* pwc = nullptr;
                    for (auto& entry : kSpecMap) {
                        if (playerClass == entry.cls && playerSpec == entry.spec) {
                            pwc = entry.wc;
                            break;
                        }
                    }

                    if (!pwc) {
                        ImGui::TextDisabled(
                            "No hardcoded spell data found for %s %s"
                            " — timeline unavailable.",
                            playerSpec.c_str(), playerClass.c_str());
                    } else if (!root.contains("events") ||
                               !root["events"].is_array() ||
                               root["events"].empty()) {
                        ImGui::TextDisabled("No cast events available for timeline.");
                    } else {
                        auto makeSet = [](const std::vector<int>& v) {
                            std::unordered_set<int> s(v.begin(), v.end());
                            return s;
                        };
                        const auto rotSet  = makeSet(pwc->getRotSpells());
                        const auto offSet  = makeSet(pwc->getOffSpells());
                        const auto defSet  = makeSet(pwc->getDefSpells());
                        const auto mobSet  = makeSet(pwc->getMobSpells());
                        const auto utilSet = makeSet(pwc->getUtilSpells());

                        std::vector<double> rotTs, offTs, defTs, mobTs, utilTs, unknownTs;

                        double t0 = -1.0, tMax = 0.0;
                        for (const auto& ev : root["events"]) {
                            if (!ev.contains("sourceID")) continue;
                            if (ev["sourceID"].get<int>() != g_debugPlayerId) continue;
                            if (ev.value("melee", false)) continue;
                            if (ev.value("fake",  false)) continue;

                            const int    sid = ev.value("abilityGameID", 0);
                            const double ts  = ev.value("timestamp", 0.0);
                            if (sid == 0 || sid == 1) continue;

                            if (t0 < 0.0) t0 = ts;
                            const double t = (ts - t0) / 1000.0;
                            tMax = std::max(tMax, t);

                            if      (rotSet.count(sid))  rotTs.push_back(t);
                            else if (offSet.count(sid))  offTs.push_back(t);
                            else if (defSet.count(sid))  defTs.push_back(t);
                            else if (mobSet.count(sid))  mobTs.push_back(t);
                            else if (utilSet.count(sid)) utilTs.push_back(t);
                            else                         unknownTs.push_back(t);
                        }

                        // ── Boss cast timeline ────────────────────────────────
                        struct BossRow { std::string name; std::vector<double> ts; };
                        std::vector<BossRow> bossRows;

                        {
                            std::string bossJson;
                            if (!g_fetchedBossData) {
                                bossJson = fetchBossCastTimeline(
                                    g_currentFight, g_cachedReportCode,
                                    g_cachedBearerToken);
                            }
                            g_fetchedBossData = true;

                            try {
                                json bj = json::parse(bossJson);
                                std::vector<std::string> bossOrder;
                                std::unordered_map<std::string, BossRow> bossMap;

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
                                        double t  = (t0 >= 0.0)
                                                     ? (ts - t0) / 1000.0
                                                     : ts / 1000.0;
                                        tMax = std::max(tMax, t);
                                        if (bossMap.find(name) != bossMap.end())
                                            bossMap[name].ts.push_back(t);
                                    }
                                }
                                for (const auto& name : bossOrder)
                                    bossRows.push_back(std::move(bossMap[name]));
                            } catch (const std::exception& e) {
                                std::cerr << "[Timeline] boss JSON parse error: "
                                          << e.what() << "\n";
                            }
                        }

                        // ── Y-axis layout ─────────────────────────────────────
                        const int kPlayerRows = 5;
                        const int kGap        = 1;
                        const int kBossBase   = kPlayerRows + kGap;
                        const int totalRows   = kBossBase + (int)bossRows.size();

                        const double yMin = -0.6;
                        const double yMax = (double)totalRows - 0.4;

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

                        bossLabelStorage.reserve(bossRows.size());
                        for (int i = 0; i < (int)bossRows.size(); ++i) {
                            bossLabelStorage.push_back(bossRows[i].name);
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
                                                    tMax > 0.0 ? tMax * 1.05 : 60.0,
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

                            plotRow(rotTs,  4.0, "##rot");
                            plotRow(offTs,  3.0, "##off");
                            plotRow(defTs,  2.0, "##def");
                            plotRow(mobTs,  1.0, "##mob");
                            plotRow(utilTs, 0.0, "##util");

                            for (int i = 0; i < (int)bossRows.size(); ++i) {
                                std::string id = "##boss" + std::to_string(i);
                                plotRow(bossRows[i].ts,
                                        (double)(kBossBase + i),
                                        id.c_str());
                            }

                            ImPlot::EndPlot();
                        }

                        // ── Cast-count summary ────────────────────────────────
                        ImGui::Spacing();
                        ImGui::Columns(5, "##castCounts", true);

                        struct CatSummary {
                            const char*              name;
                            const std::vector<double>* ts;
                            ImVec4                   col;
                        };
                        const CatSummary kSummary[] = {
                            { "Rotation",  &rotTs,  ImVec4(1.00f,0.35f,0.35f,1.0f) },
                            { "Offensive", &offTs,  ImVec4(1.00f,0.70f,0.10f,1.0f) },
                            { "Defensive", &defTs,  ImVec4(0.30f,0.70f,1.00f,1.0f) },
                            { "Mobility",  &mobTs,  ImVec4(0.40f,1.00f,0.40f,1.0f) },
                            { "Utility",   &utilTs, ImVec4(0.90f,0.60f,1.00f,1.0f) },
                        };
                        for (auto& s : kSummary) {
                            ImGui::TextColored(s.col, "%s", s.name);
                            ImGui::Text("%d casts", (int)s.ts->size());
                            ImGui::NextColumn();
                        }
                        ImGui::Columns(1);

                        if (!unknownTs.empty()) {
                            ImGui::Spacing();
                            ImGui::TextDisabled(
                                "(%d casts not matched to any spell list"
                                " — check specData.h)",
                                (int)unknownTs.size());
                        }
                    }

                } catch (const json::exception& e) {
                    ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f),
                                       "JSON parse error: %s", e.what());
                }
            }

            ImGui::End();
        }

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