#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <filesystem>
#include <vector>
#include <stdexcept>
#include <thread>
#include <atomic>
#include <optional>
#include <mutex>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <unistd.h>
    #include <limits.h>
#endif

#include "specData.h"

#include "external/imgui-1.92.7/imgui.h"
#include "external/imgui-1.92.7/backends/imgui_impl_glfw.h"
#include "external/imgui-1.92.7/backends/imgui_impl_opengl3.h"
#include "external/glfw-3.4/include/GLFW/glfw3.h"
#include "external/curl-8.19.0/include/curl/curl.h"
#include "external/nlohmann/json.hpp"

#include "imPlotWrapper.h"

using json = nlohmann::json;

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

struct ReportData
{
    std::string title;
    std::string owner;
    std::vector<Fight> fights;
};

std::string getExecutableDirectory() {
    char buffer[1024];
    
    #ifdef _WIN32
        GetModuleFileNameA(NULL, buffer, sizeof(buffer));
    #else
        ssize_t len = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
        if (len != -1) {
            buffer[len] = '\0';
        } else {
            return ".";
        }
    #endif
    
    std::filesystem::path exePath(buffer);
    return exePath.parent_path().string();
}

#define KEY_FILE_DIRECTORY (std::string(getExecutableDirectory()) + "/APIkey.txt")

std::vector<std::string> fetchKeys(const std::string& filePath) {
    std::ifstream file(filePath);

    if (!file.is_open()) {
        std::ofstream newFile(filePath);
        if (!newFile.is_open()) {
            throw std::runtime_error("Failed to create file: " + filePath);
        }
        newFile << "ClientID=your_client_id_here\n";
        newFile << "ClientSecret=your_client_secret_here\n";
        newFile.close();
        throw std::runtime_error("File created at: " + filePath + ". Please populate the ClientID and ClientSecret fields.");
    }

    std::string line;
    std::string clientID, clientSecret;
    const std::string idPrefix     = "ClientID=";
    const std::string secretPrefix = "ClientSecret=";

    while (std::getline(file, line)) {
        size_t pos;
        if ((pos = line.find(idPrefix)) != std::string::npos) {
            clientID = line.substr(pos + idPrefix.length());
        } else if ((pos = line.find(secretPrefix)) != std::string::npos) {
            clientSecret = line.substr(pos + secretPrefix.length());
        }
    }

    if (clientID.empty()) {
        throw std::runtime_error("ClientID not found in file: " + filePath);
    }
    if (clientSecret.empty()) {
        throw std::runtime_error("ClientSecret not found in file: " + filePath);
    }

    return { clientID, clientSecret };
}

void appendToken(const std::string& filePath, const std::string& token) {
    std::ofstream file(filePath, std::ios::app);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + filePath);
    }
    file << "token=" << token << "\n";
}

std::string ExtractReportCode(const std::string& input) {
    const std::string marker = "/reports/";
    size_t pos = input.find(marker);
    if (pos == std::string::npos)
        return input;

    std::string code = input.substr(pos + marker.size());

    for (char stop : { '#', '?' }) {
        size_t cut = code.find(stop);
        if (cut != std::string::npos)
            code = code.substr(0, cut);
    }
    return code;
}

static size_t CurlWriteCallback(void* contents, size_t size, size_t nmemb, std::string* out) {
    out->append((char*)contents, size * nmemb);
    return size * nmemb;
}

std::string FetchOAuthToken(const std::string& clientId, const std::string& clientSecret) {

    std::string line;
    std::ifstream file(KEY_FILE_DIRECTORY);
    std::string response;
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + KEY_FILE_DIRECTORY);
    } else {
        while (std::getline(file , line)){
            size_t pos;
            if ((pos = line.find("token=")) != std::string::npos){
                response = line.substr(pos + 6);
                return(response);
            }
        }
    }

    CURL* curl = curl_easy_init();
    if (!curl) return "";

    std::string credentials = clientId + ":" + clientSecret;
    std::string postFields = "grant_type=client_credentials";

    curl_easy_setopt(curl, CURLOPT_URL, "https://www.warcraftlogs.com/oauth/token");
    curl_easy_setopt(curl, CURLOPT_USERPWD, credentials.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postFields.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);

    curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    return response;
}

// Helper to perform a single curl POST and return the response body
static std::string curlPost(const std::string& url,
                             const std::string& bearerToken,
                             const std::string& jsonBody)
{
    CURL* curl = curl_easy_init();
    if (!curl) return "[error] curl_easy_init() failed";

    std::string response;
    struct curl_slist* headers = nullptr;
    std::string authHeader = "Authorization: Bearer " + bearerToken;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, authHeader.c_str());

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonBody.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK)
        response = std::string("[curl error] ") + curl_easy_strerror(res);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    return response;
}

static std::string jsonEscape(const std::string& s)
{
    std::string out;
    for (char c : s) {
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:   out += c;
        }
    }
    return out;
}

std::string FetchReportData(const std::string& bearerToken, const std::string& reportCode) {
    const std::string API_URL = "https://www.warcraftlogs.com/api/v2/client";

    std::string query =
        "{\"query\":\"{ reportData { report(code: \\\"" + jsonEscape(reportCode) + "\\\") {"
        "  title startTime endTime"
        "  owner { name }"
        "  playerDetails(startTime: 0, endTime: 99999999999)"
        "  fights {"
        "    id name startTime endTime kill difficulty bossPercentage"
        "    friendlyPlayers"
        "  }"
        "} } }\"}";

    std::cout << "[WCL] FetchReportData query:\n" << query << "\n";

    std::string response = curlPost(API_URL, bearerToken, query);

    //std::cout << "[WCL] FetchReportData response:\n" << response << "\n";

    try
    {
        json root = json::parse(response);
        json& report = root["data"]["reportData"]["report"];

        ReportData rd;
        rd.title = report.value("title", "");
        rd.owner = report["owner"].value("name", "");

        // playerDetails comes back as a JSON object keyed by role
        json& pd = report["playerDetails"]["data"]["playerDetails"];

        // Build a quick id->name lookup from every role bucket
        std::unordered_map<int, std::string> idToName;
        for (auto& role : { "tanks", "healers", "dps" })
        {
            if (!pd.contains(role)) continue;
            for (auto& p : pd[role])
            {
                int pid = p.value("id", -1);
                if (pid != -1)
                    idToName[pid] = p.value("name", "");
            }
        }

        for (auto& f : report["fights"])
        {
            // Skip trash / non-boss entries that have no name
            if (!f.contains("id")) continue;

            Fight fight;
            fight.id        = f.value("id", 0);
            fight.name      = f.value("name", "");
            fight.startTime = f.value("startTime", 0.0);
            fight.endTime   = f.value("endTime",   0.0);
            fight.percent   = f.value("bossPercentage", 0.0f);

            if (!f["kill"].is_null())
                fight.kill = f["kill"].get<bool>();

            if (!f["difficulty"].is_null())
                fight.difficulty = f["difficulty"].get<int>();

            // Map friendlyPlayers ids -> PlayerEntry, bucketed by role
            if (f.contains("friendlyPlayers"))
            {
                for (int pid : f["friendlyPlayers"])
                {
                    auto it = idToName.find(pid);
                    std::string pname = (it != idToName.end()) ? it->second : "";

                    // Determine role from playerDetails buckets
                    auto inBucket = [&](const char* role) {
                        if (!pd.contains(role)) return false;
                        for (auto& p : pd[role])
                            if (p.value("id", -1) == pid) return true;
                        return false;
                    };

                    PlayerEntry pe{ pid, pname };
                    if      (inBucket("tanks"))   fight.tanks.push_back(pe);
                    else if (inBucket("healers"))  fight.healers.push_back(pe);
                    else if (inBucket("dps"))      fight.dps.push_back(pe);
                }
            }

            rd.fights.push_back(std::move(fight));
        }

        //if needed, store previous request command will go here
    }
    catch (const json::exception& e)
    {
        std::cerr << "[WCL] FetchReportData JSON parse error: " << e.what() << "\n";
    }

    std::cout << "[DEBUG]: FetchReportData() output:\n" + response + "\n";
    return response;
}

std::string fetchPersonalFightData(const Fight&       fight,
                                   int                playerId,
                                   const std::string& reportCode,
                                   const std::string& bearerToken)
{
    const std::string API_URL = "https://www.warcraftlogs.com/api/v2/client";

    const std::string startStr = std::to_string(static_cast<long long>(fight.startTime));
    const std::string endStr   = std::to_string(static_cast<long long>(fight.endTime));
    const std::string pidStr   = std::to_string(playerId);
    const std::string code     = jsonEscape(reportCode);

    json result;

    // ----------------------------------------------------------------
    //  1. Table query  (gear, stats, aggregated damage/healing totals)
    // ----------------------------------------------------------------
    {
        std::string query =
            "{\"query\":\"{ reportData { report(code: \\\"" + code + "\\\") {"
            "  table("
            "    startTime: " + startStr + ","
            "    endTime: "   + endStr   + ","
            "    sourceID: "  + pidStr   +
            "  )"
            "} } }\"}";

        //std::cout << "[WCL] table query:\n" << query << "\n";

        std::string tableResponse = curlPost(API_URL, bearerToken, query);

        //std::cout << "[WCL] table response:\n" << tableResponse << "\n";

        try
        {
            json tableJson = json::parse(tableResponse);
            result["table"] = tableJson["data"]["reportData"]["report"]["table"];
        }
        catch (const json::exception& e)
        {
            std::cerr << "[WCL] table JSON parse error: " << e.what() << "\n";
            result["table"] = nullptr;
        }
    }

    // ----------------------------------------------------------------
    //  2. Events query  (individual cast events, paginated)
    //     All pages are merged into a single "events" array in result.
    // ----------------------------------------------------------------
    {
        long long pageStart      = static_cast<long long>(fight.startTime);
        const long long fightEnd = static_cast<long long>(fight.endTime);
        int page = 0;

        result["events"] = json::array();

        while (true)
        {
            std::string query =
                "{\"query\":\"{ reportData { report(code: \\\"" + code + "\\\") {"
                "  events("
                "    startTime: " + std::to_string(pageStart) + ","
                "    endTime: "   + endStr                     + ","
                "    sourceID: "  + pidStr                      + ","
                "    filterExpression: \\\"type = 'cast'\\\""
                "  ) {"
                "    data"
                "    nextPageTimestamp"
                "  }"
                "} } }\"}";

            std::cout << "[WCL] events page " << page << " query:\n" << query << "\n";

            std::string pageResponse = curlPost(API_URL, bearerToken, query);

            //std::cout << "[WCL] events page " << page << " response:\n" << pageResponse << "\n";

            try
            {
                json pageJson  = json::parse(pageResponse);
                json& events   = pageJson["data"]["reportData"]["report"]["events"];

                // Merge this page's events into the result array
                for (auto& event : events["data"])
                    result["events"].push_back(event);

                if (events["nextPageTimestamp"].is_null()) break;

                long long nextTs = events["nextPageTimestamp"].get<long long>();
                if (nextTs <= pageStart || nextTs >= fightEnd) break;

                pageStart = nextTs;
                ++page;
            }
            catch (const json::exception& e)
            {
                std::cerr << "[WCL] events page " << page
                          << " JSON parse error: " << e.what() << "\n";
                break;
            }
        }
    }

    std::cout << "[DEBUG]: fetchPersonalFightData() output:\n" + result.dump() + "\n";
    return result.dump();
}

// ---------------------------------------------------------------------------
// Global state shared between the main thread and the background fetch thread
// ---------------------------------------------------------------------------
static ReportData            g_report;
static std::mutex            g_reportMutex;
static std::atomic<bool>     g_reportReady{ false };

// Cached after the first successful report fetch so that player-button clicks
// can re-use the same credentials without threading headaches.
static std::string           g_cachedReportCode;
static std::string           g_cachedBearerToken;

// Debug window for per-player data
static std::mutex            g_debugMutex;
static bool                  g_debugWindowOpen   = false;
static std::atomic<bool>     g_debugFetching{ false };
static std::string           g_debugWindowTitle;   // "PlayerName — FightName"
static std::string           g_debugRawResponse;   // verbatim server JSON
static int                   g_debugPlayerId = -1;

int main(){
    static AllSpecs masterSpecList;
    static std::string tokenResponse;
    static bool tokenFetched = false;
    static char wclUrlBuf[512] = "";
    static std::atomic<bool> isFetching(false);
    static std::string fetchStatus;
    std::vector<std::string> keys;
    std::string errorMsg;

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
    
        CURL* curl = curl_easy_init();
        if (!curl) return;

        std::string responseBuffer;
        std::string credentials = keys[0] + ":" + keys[1];
        std::string postFields = "grant_type=client_credentials";

        curl_easy_setopt(curl, CURLOPT_CAINFO, "cacert.pem");
        curl_easy_setopt(curl, CURLOPT_URL, "https://www.warcraftlogs.com/oauth/token");
        curl_easy_setopt(curl, CURLOPT_USERPWD, credentials.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postFields.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, +[](char* ptr, size_t size, size_t nmemb, void* userdata) -> size_t {
            auto* buf = static_cast<std::string*>(userdata);
            buf->append(ptr, size * nmemb);
            return size * nmemb;
        });
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBuffer);

        CURLcode res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);

        if (res == CURLE_OK) {
            try {
                auto j = nlohmann::json::parse(responseBuffer);
                if (j.contains("access_token"))
                    tokenResponse = j["access_token"].get<std::string>();
                else
                    tokenResponse = responseBuffer;
            } catch (...) {
                tokenResponse = "JSON parse error: " + responseBuffer;
            }
        } else {
            tokenResponse = std::string("CURL error: ") + curl_easy_strerror(res);
        }

        if (tokenResponse[0] != '{'){
            appendToken(KEY_FILE_DIRECTORY , tokenResponse);
            tokenFetched = true;
        }
    };

    try {
        keys = fetchKeys(KEY_FILE_DIRECTORY);
    } catch (const std::exception& e) {
        errorMsg = e.what();
    }

    // Initialize GLFW
    if (!glfwInit()){return -1;}

    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    GLFWwindow* window = glfwCreateWindow(960, 400, "Beard Tools", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    // Initialize ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    static const ImVec4 s_pieColors[] = {
        ImVec4(1.0f, 0.4f, 0.4f, 1.0f), // Crit
        ImVec4(0.4f, 1.0f, 0.4f, 1.0f), // Haste
        ImVec4(0.4f, 0.8f, 1.0f, 1.0f), // Mastery
        ImVec4(0.9f, 0.6f, 1.0f, 1.0f), // Versatility
    };
    ImPlotColormap g_secondariesColormap = ImPlot::AddColormap("secondaries color", s_pieColors, 4);

    FetchToken();

    // Main render loop
    while (!glfwWindowShouldClose(window)) {
        bool currentlyFetching = isFetching.load();
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGuiIO& io = ImGui::GetIO();
        ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f),
                                ImGuiCond_Always, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(880, 320), ImGuiCond_Always);

        ImGui::Begin("Main Window", nullptr,
            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

        if (!errorMsg.empty()) {
            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Error: %s", errorMsg.c_str());
        } else {
            ImGui::Text("Loaded key from: %s", KEY_FILE_DIRECTORY.c_str());
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // --- WCL URL input row ---
        ImGui::Text("WCL Report URL:");
        ImGui::SetNextItemWidth(700.0f);
        ImGui::InputText("##wclurl", wclUrlBuf, sizeof(wclUrlBuf));

        ImGui::SameLine();

        if (currentlyFetching) ImGui::BeginDisabled();

        if (ImGui::Button("Fetch Log")) {
            std::string urlCopy      = wclUrlBuf;
            std::string clientId     = keys[0];
            std::string clientSecret = keys[1];

            isFetching = true;
            fetchStatus = "Fetching...";

            std::thread([urlCopy, clientId, clientSecret]() {
                std::string reportCode = ExtractReportCode(urlCopy);

                std::string tokenJson  = FetchOAuthToken(clientId, clientSecret);
                std::string reportJson = FetchReportData(tokenJson, reportCode);

                try {
                    auto root = nlohmann::json::parse(reportJson);

                    if (!root.contains("data") || root["data"].is_null() ||
                        !root["data"].contains("reportData") || root["data"]["reportData"].is_null() ||
                        !root["data"]["reportData"].contains("report") || root["data"]["reportData"]["report"].is_null()) {
                        fetchStatus = "API error: " + reportJson;
                        std::cout << "API error: " + reportJson;
                        isFetching = false;
                        return;
                    }

                    auto& rep = root["data"]["reportData"]["report"];

                    ReportData parsed;
                    parsed.title = rep.contains("title") && rep["title"].is_string() ? rep["title"].get<std::string>() : "";
                    parsed.owner = (rep.contains("owner") && rep["owner"].is_object() && rep["owner"].contains("name"))
                                    ? rep["owner"]["name"].get<std::string>() : "";

                    // --- Build id -> {name, role} lookup from playerDetails ---
                    struct PlayerInfo { std::string name; std::string role; };
                    std::unordered_map<int, PlayerInfo> playerLookup;

                    if (rep.contains("playerDetails") && !rep["playerDetails"].is_null()) {
                        nlohmann::json pd;
                        if (rep["playerDetails"].is_string()) {
                            pd = nlohmann::json::parse(rep["playerDetails"].get<std::string>());
                        } else {
                            pd = rep["playerDetails"];
                        }

                        [&]() {
                            if (!pd.contains("data") || !pd["data"].is_object()) return;
                            if (!pd["data"].contains("playerDetails") || !pd["data"]["playerDetails"].is_object()) return;

                            auto& pdInner = pd["data"]["playerDetails"];
                            for (const char* role : {"tanks", "healers", "dps"}) {
                                if (!pdInner.contains(role) || !pdInner[role].is_array()) continue;
                                for (auto& p : pdInner[role]) {
                                    if (!p.is_object()) continue;
                                    if (!p.contains("id") || !p["id"].is_number()) continue;

                                    int pid = p["id"].get<int>();
                                    std::string pname = (p.contains("name") && p["name"].is_string())
                                                            ? p["name"].get<std::string>() : "?";
                                    playerLookup[pid] = { pname, role };
                                }
                            }
                        }();
                    }

                    // --- Parse fights ---
                    if (!rep.contains("fights") || !rep["fights"].is_array()) {
                        fetchStatus = "No fights data in response";
                        isFetching = false;
                        return;
                    }

                    for (auto& f : rep["fights"]) {
                        if (!f.is_object()) continue;

                        Fight fight;
                        fight.id        = f.contains("id")        && f["id"].is_number()        ? f["id"].get<int>()           : 0;
                        fight.name      = f.contains("name")      && f["name"].is_string()      ? f["name"].get<std::string>() : "";
                        fight.startTime = f.contains("startTime") && f["startTime"].is_number() ? f["startTime"].get<double>() : 0.0;
                        fight.endTime   = f.contains("endTime")   && f["endTime"].is_number()   ? f["endTime"].get<double>()   : 0.0;
                        fight.percent   = f.contains("bossPercentage") && f["bossPercentage"].is_number()
                                            ? f["bossPercentage"].get<double>() : 0.0;

                        if (!f.contains("kill")       || f["kill"].is_null())       { continue; }
                        if (!f.contains("difficulty") || f["difficulty"].is_null()) { continue; }

                        fight.kill       = f["kill"].get<bool>();
                        fight.difficulty = f["difficulty"].get<int>();

                        // --- Cross-reference friendlyPlayers against lookup ---
                        // Store both the numeric id and the name so buttons can
                        // hand the id straight to fetchPersonalFightData().
                        if (f.contains("friendlyPlayers") && f["friendlyPlayers"].is_array()) {
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
                }
                catch (const std::exception& e) {
                    fetchStatus = std::string("Parse error: ") + e.what();
                }

                isFetching = false;
            }).detach();
        }

        if (currentlyFetching) ImGui::EndDisabled();

        // Status line
        if (!fetchStatus.empty()) {
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(0.6f, 0.9f, 0.6f, 1.0f), "%s", fetchStatus.c_str());
        }

        // --- Fight list ---
        if (g_reportReady.load()) {
            std::lock_guard<std::mutex> lock(g_reportMutex);

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            ImGui::Text("Report: %s  (owner: %s)", g_report.title.c_str(), g_report.owner.c_str());
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
                if (!kill)  return ImVec4(0.35f, 0.35f, 0.35f, 1.0f);
                if (*kill)  return ImVec4(0.10f, 0.45f, 0.10f, 1.0f);
                return           ImVec4(0.50f, 0.10f, 0.10f, 1.0f);
            };
            auto headerColorHov = [](std::optional<bool> kill) -> ImVec4 {
                if (!kill)  return ImVec4(0.45f, 0.45f, 0.45f, 1.0f);
                if (*kill)  return ImVec4(0.15f, 0.60f, 0.15f, 1.0f);
                return           ImVec4(0.65f, 0.15f, 0.15f, 1.0f);
            };
            auto headerColorAct = [](std::optional<bool> kill) -> ImVec4 {
                if (!kill)  return ImVec4(0.55f, 0.55f, 0.55f, 1.0f);
                if (*kill)  return ImVec4(0.20f, 0.70f, 0.20f, 1.0f);
                return           ImVec4(0.75f, 0.20f, 0.20f, 1.0f);
            };

            // Helper that renders a row of player buttons and fires off a
            // background fetch when any of them is clicked.
            // Capture by value so the lambda is safe to call from the thread.
            auto renderPlayerButtons = [&](
                const std::vector<PlayerEntry>& players,
                const Fight&                    fight,
                const char*                     roleTag,
                int                             wrapAfter)
            {
                bool fetching = g_debugFetching.load();

                for (int i = 0; i < (int)players.size(); ++i) {
                    const PlayerEntry& pe = players[i];

                    std::string btnId = pe.name
                                    + "##" + roleTag
                                    + "_f" + std::to_string(fight.id)
                                    + "_p" + std::to_string(pe.id);

                    bool wasDisabled = fetching;          // snapshot BEFORE button + click handler
                    if (wasDisabled) ImGui::BeginDisabled();

                    if (ImGui::Button(btnId.c_str())) {
                        Fight       capturedFightCopy = fight;
                        std::string capturedName      = pe.name;
                        std::string capturedFight     = fight.name;
                        int         capturedPid       = pe.id;
                        std::string capturedCode      = g_cachedReportCode;
                        std::string capturedToken     = g_cachedBearerToken;

                        {
                            std::lock_guard<std::mutex> dlock(g_debugMutex);
                            g_debugWindowTitle = capturedName + " — " + capturedFight;
                            g_debugRawResponse = "Fetching\xe2\x80\xa6";
                            g_debugWindowOpen  = true;
                            g_debugPlayerId    = capturedPid; 
                        }
                        g_debugFetching = true;
                        fetching        = true;           // update for NEXT iteration's BeginDisabled

                        std::thread([capturedFightCopy, capturedPid,
                                    capturedCode, capturedToken]() {
                            std::string raw = fetchPersonalFightData(
                                capturedFightCopy, capturedPid, capturedCode, capturedToken);
                            {
                                std::lock_guard<std::mutex> dlock(g_debugMutex);
                                g_debugRawResponse = raw;
                            }
                            g_debugFetching = false;
                        }).detach();
                    }

                    if (wasDisabled) ImGui::EndDisabled(); // matches the Begin above — always paired

                    if (wrapAfter > 0 && (i + 1) % wrapAfter != 0 && i + 1 < (int)players.size())
                        ImGui::SameLine();
                }
            };

            ImGui::BeginChild("##fights", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

            for (auto& fight : g_report.fights) {
                double durationSec = (fight.endTime - fight.startTime) / 1000.0;
                int    mins        = static_cast<int>(durationSec) / 60;
                int    secs        = static_cast<int>(durationSec) % 60;

                const char* killStr = fight.kill ? (*fight.kill ? "Kill" : "Wipe") : "—";
                char label[256];
                char percentStr[16];
                snprintf(percentStr, sizeof(percentStr), "%.2f%%", fight.percent);

                snprintf(label, sizeof(label), "[%d] %s  |  %s  |  %d:%02d  |  %s | %s",
                    fight.id,
                    fight.name.c_str(),
                    diffLabel(fight.difficulty),
                    mins, secs,
                    killStr,
                    percentStr);

                ImGui::PushStyleColor(ImGuiCol_Header,        headerColor(fight.kill));
                ImGui::PushStyleColor(ImGuiCol_HeaderHovered, headerColorHov(fight.kill));
                ImGui::PushStyleColor(ImGuiCol_HeaderActive,  headerColorAct(fight.kill));

                if (ImGui::CollapsingHeader(label)) {
                    ImGui::Indent(16.0f);

                    // --- Tanks ---
                    if (!fight.tanks.empty()) {
                        ImGui::TextColored(ImVec4(0.3f, 0.6f, 1.0f, 1.0f), "Tanks");
                        ImGui::Spacing();
                        renderPlayerButtons(fight.tanks, fight, "tank", 0 /*no wrap*/);
                        ImGui::NewLine();
                        ImGui::Spacing();
                    }

                    // --- Healers ---
                    if (!fight.healers.empty()) {
                        ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.5f, 1.0f), "Healers");
                        ImGui::Spacing();
                        renderPlayerButtons(fight.healers, fight, "heal", 5);
                        ImGui::NewLine();
                        ImGui::Spacing();
                    }

                    // --- DPS ---
                    if (!fight.dps.empty()) {
                        ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "DPS");
                        ImGui::Spacing();
                        renderPlayerButtons(fight.dps, fight, "dps", 5);
                        ImGui::NewLine();
                        ImGui::Spacing();
                    }

                    if (fight.tanks.empty() && fight.healers.empty() && fight.dps.empty()) {
                        ImGui::TextDisabled("No player data available.");
                    }

                    ImGui::Unindent(16.0f);
                    ImGui::Spacing();
                }

                ImGui::PopStyleColor(3);
            }

            ImGui::EndChild();
        }

        ImGui::End();

        // -----------------------------------------------------------------------
        // Per-player window
        // The window is resizable and scrollable so large payloads stay readable.
        // -----------------------------------------------------------------------
        if (g_debugWindowOpen) {
            std::lock_guard<std::mutex> dlock(g_debugMutex);

            ImGui::SetNextWindowSize(ImVec2(720, 600), ImGuiCond_Appearing);
            ImGui::SetNextWindowPos(
                ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f),
                ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

            std::string windowTitle = "[DEBUG] " + g_debugWindowTitle;
            ImGui::Begin(windowTitle.c_str(), &g_debugWindowOpen);

            if (g_debugFetching.load()) {
                ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "Fetching from server...");
            } else {
                try {
                    auto root       = json::parse(g_debugRawResponse);
                    auto& tableData = root["table"]["data"];
                    auto& info      = tableData["combatantInfo"];
                    auto& stats     = info["stats"];

                    // ── Resolve player identity from composition using cached player ID ──
                    std::string playerName  = "Unknown";
                    std::string playerClass = "Unknown";
                    std::string playerSpec  = "Unknown";
                    std::string playerRole  = "Unknown";

                    if (tableData.contains("composition") && tableData["composition"].is_array()) {
                        for (auto& member : tableData["composition"]) {
                            if (!member.contains("id") || !member["id"].is_number()) continue;
                            if (member["id"].get<int>() != g_debugPlayerId) continue;

                            playerName  = member.value("name", "Unknown");
                            playerClass = member.value("type", "Unknown");

                            if (member.contains("specs") && member["specs"].is_array()
                                && !member["specs"].empty()) {
                                auto& spec = member["specs"][0];
                                playerSpec = spec.value("spec", "Unknown");
                                playerRole = spec.value("role", "Unknown");
                            }
                            break;
                        }
                    }

                    float ilvl = tableData.value("itemLevel", 0.0f);

                    // ── Name / Class / Spec banner ───────────────────────────────────────
                    ImGui::Separator();
                    ImGui::TextColored(ImVec4(0.3f, 0.9f, 0.5f, 1.0f), "%s", playerName.c_str());
                    ImGui::SameLine();
                    ImGui::TextDisabled("(%s %s  |  Role: %s)",
                                        playerSpec.c_str(), playerClass.c_str(), playerRole.c_str());
                    ImGui::Separator();

                    // ── Item Level ───────────────────────────────────────────────────────
                    ImGui::Text("Item Level");
                    ImGui::SameLine(160);
                    ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.0f, 1.0f), "%.1f", ilvl);

                    ImGui::Spacing();
                    ImGui::Separator();

                    // ── Primary Stats ────────────────────────────────────────────────────
                    ImGui::TextColored(ImVec4(0.7f, 0.7f, 1.0f, 1.0f), "Primary Stats");
                    ImGui::Separator();

                    auto printStat = [&](const char* label, const char* key,
                                        ImVec4 col = ImVec4(1,1,1,1)) {
                        if (stats.contains(key) && stats[key].contains("max")) {
                            int val = stats[key]["max"].get<int>();
                            ImGui::Text("  %s", label);
                            ImGui::SameLine(160);
                            ImGui::TextColored(col, "%d", val);
                        }
                    };

                    printStat("Agility",   "Agility",   ImVec4(1.0f, 0.75f, 0.2f, 1.0f));
                    printStat("Strength",  "Strength",  ImVec4(1.0f, 0.75f, 0.2f, 1.0f));
                    printStat("Intellect", "Intellect", ImVec4(1.0f, 0.75f, 0.2f, 1.0f));
                    printStat("Stamina",   "Stamina",   ImVec4(1.0f, 0.75f, 0.2f, 1.0f));

                    ImGui::Spacing();

                    // ── Secondary Stats ──────────────────────────────────────────────────
                    ImGui::TextColored(ImVec4(0.7f, 0.7f, 1.0f, 1.0f), "Secondary Stats");
                    ImGui::Separator();

                    printStat("Crit",         "Crit",         ImVec4(1.0f, 0.4f, 0.4f, 1.0f));
                    printStat("Haste",        "Haste",        ImVec4(0.4f, 1.0f, 0.4f, 1.0f));
                    printStat("Mastery",      "Mastery",      ImVec4(0.4f, 0.8f, 1.0f, 1.0f));
                    printStat("Versatility",  "Versatility",  ImVec4(0.9f, 0.6f, 1.0f, 1.0f));

                    ImGui::Spacing();

                    // ── Secondary Stats Pie Chart ────────────────────────────────────────
                    const char* labels[] = { "##0", "##1", "##2", "##3" };
                    const char* keys[]   = { "Crit", "Haste", "Mastery", "Versatility" };

                    double values[4] = {};
                    for (int i = 0; i < 4; ++i) {
                        if (stats.contains(keys[i]) && stats[keys[i]].contains("max"))
                            values[i] = stats[keys[i]]["max"].get<int>();
                    }

                    ImPlot::PushColormap(g_secondariesColormap);

                    if (ImPlot::BeginPlot("##PieChart", ImVec2(250, 250),
                                        ImPlotFlags_Equal | ImPlotFlags_NoMouseText | ImPlotFlags_NoInputs | ImPlotFlags_NoLegend))
                    {
                        ImPlot::SetupAxes(nullptr, nullptr,
                                        ImPlotAxisFlags_NoDecorations,
                                        ImPlotAxisFlags_NoDecorations);

                        ImPlot::PlotPieChart(labels, values, 4,
                                            /*x=*/0.5, /*y=*/0.5, /*radius=*/0.4,
                                            "%.0f");
                        ImPlot::EndPlot();
                    }

                    ImPlot::PopColormap();

                    ImGui::Spacing();

                    // ── Defensive Stats ──────────────────────────────────────────────────
                    ImGui::TextColored(ImVec4(0.7f, 0.7f, 1.0f, 1.0f), "Defensive Stats");
                    ImGui::Separator();

                    printStat("Armor",      "Armor",      ImVec4(0.8f, 0.8f, 0.8f, 1.0f));
                    printStat("Leech",      "Leech",      ImVec4(0.6f, 1.0f, 0.6f, 1.0f));
                    printStat("Avoidance",  "Avoidance",  ImVec4(0.8f, 0.8f, 0.8f, 1.0f));
                    printStat("Speed",      "Speed",      ImVec4(0.8f, 0.8f, 0.8f, 1.0f));

                    ImGui::Spacing();
                    ImGui::Separator();

                } catch (const json::exception& e) {
                    ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f),
                                    "JSON parse error: %s", e.what());
                }
            }

            ImGui::End();
        }

        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}