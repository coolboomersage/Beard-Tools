#include "api.h"
#include "utils.h"

#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>

#include "external/curl-8.19.0/include/curl/curl.h"
#include "external/nlohmann/json.hpp"

using json = nlohmann::json;

// ── Internal curl helpers (not exposed in the header) ────────────────────────

static size_t CurlWriteCallback(void* contents, size_t size, size_t nmemb,
                                std::string* out) {
    out->append(static_cast<char*>(contents), size * nmemb);
    return size * nmemb;
}

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

    curl_easy_setopt(curl, CURLOPT_URL,           url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER,    headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS,    jsonBody.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA,     &response);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK)
        response = std::string("[curl error] ") + curl_easy_strerror(res);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    return response;
}

static std::string jsonEscape(const std::string& s) {
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

// ── Public API ────────────────────────────────────────────────────────────────

std::string FetchOAuthToken(const std::string& clientId,
                            const std::string& clientSecret)
{
    // Check for a cached token in the key file first.
    std::ifstream file(KEY_FILE_DIRECTORY);
    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line)) {
            size_t pos;
            if ((pos = line.find("token=")) != std::string::npos)
                return line.substr(pos + 6);
        }
    }

    // Fall back to OAuth client-credentials flow.
    CURL* curl = curl_easy_init();
    if (!curl) return "";

    std::string response;
    std::string credentials = clientId + ":" + clientSecret;
    std::string postFields  = "grant_type=client_credentials";

    curl_easy_setopt(curl, CURLOPT_URL,      "https://www.warcraftlogs.com/oauth/token");
    curl_easy_setopt(curl, CURLOPT_USERPWD,  credentials.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postFields.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);

    curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    return response;
}

std::string FetchReportData(const std::string& bearerToken,
                            const std::string& reportCode)
{
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
    //std::cout << "[DEBUG]: FetchReportData() output:\n" << response << "\n";
    return response;
}

std::string fetchPersonalFightData(const Fight&       fight,
                                   int                playerId,
                                   const std::string& reportCode,
                                   const std::string& bearerToken)
{
    const std::string API_URL  = "https://www.warcraftlogs.com/api/v2/client";
    const std::string startStr = std::to_string(static_cast<long long>(fight.startTime));
    const std::string endStr   = std::to_string(static_cast<long long>(fight.endTime));
    const std::string pidStr   = std::to_string(playerId);
    const std::string code     = jsonEscape(reportCode);

    json result;

    // ── 1. Table query (gear, stats, aggregated totals) ───────────────────────
    {
        std::string query =
            "{\"query\":\"{ reportData { report(code: \\\"" + code + "\\\") {"
            "  table("
            "    startTime: " + startStr + ","
            "    endTime: "   + endStr   + ","
            "    sourceID: "  + pidStr   +
            "  )"
            "} } }\"}";

        std::string tableResponse = curlPost(API_URL, bearerToken, query);
        try {
            json tableJson = json::parse(tableResponse);
            result["table"] = tableJson["data"]["reportData"]["report"]["table"];
        } catch (const json::exception& e) {
            std::cerr << "[WCL] table JSON parse error: " << e.what() << "\n";
            result["table"] = nullptr;
        }
    }

    // ── 2. Events query (individual cast events, paginated) ───────────────────
    {
        long long       pageStart = static_cast<long long>(fight.startTime);
        const long long fightEnd  = static_cast<long long>(fight.endTime);
        int             page      = 0;

        result["events"] = json::array();

        while (true) {
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
            try {
                json pageJson = json::parse(pageResponse);
                json& events  = pageJson["data"]["reportData"]["report"]["events"];

                for (auto& event : events["data"])
                    result["events"].push_back(event);

                if (events["nextPageTimestamp"].is_null()) break;

                long long nextTs = events["nextPageTimestamp"].get<long long>();
                if (nextTs <= pageStart || nextTs >= fightEnd) break;

                pageStart = nextTs;
                ++page;
            } catch (const json::exception& e) {
                std::cerr << "[WCL] events page " << page
                          << " JSON parse error: " << e.what() << "\n";
                break;
            }
        }
    }

    //std::cout << "[DEBUG]: fetchPersonalFightData() output:\n" << result.dump() << "\n";
    return result.dump();
}

std::string fetchBossCastTimeline(const Fight&       fight,
                                  const std::string& reportCode,
                                  const std::string& bearerToken)
{
    const std::string API_URL = "https://www.warcraftlogs.com/api/v2/client";
    const std::string code    = jsonEscape(reportCode);

    // ── 1. Resolve all boss sourceIDs + names from masterData actors ──────────
    struct BossActor { int id; std::string name; };
    std::vector<BossActor> bossActors;

    {
        std::string query =
            "{\"query\":\"{ reportData { report(code: \\\"" + code + "\\\") {"
            "  masterData {"
            "    actors(type: \\\"NPC\\\") {"
            "      id name subType"
            "    }"
            "  }"
            "} } }\"}";

        std::string response = curlPost(API_URL, bearerToken, query);
        try {
            json j = json::parse(response);
            const auto& actors =
                j["data"]["reportData"]["report"]["masterData"]["actors"];

            for (const auto& actor : actors) {
                if (actor.value("subType", "") == "Boss") {
                    bossActors.push_back({
                        actor["id"].get<int>(),
                        actor.value("name", "Unknown Boss")
                    });
                }
            }
        } catch (const json::exception& e) {
            std::cerr << "[WCL] masterData parse error: " << e.what() << "\n";
            return "{}";
        }

        if (bossActors.empty()) {
            std::cerr << "[WCL] Could not find any boss actors in masterData.\n";
            return "{}";
        }
    }

    // ── 2. Fetch ability names for human-readable output ──────────────────────
    json abilityMap = json::object();
    {
        std::string query =
            "{\"query\":\"{ reportData { report(code: \\\"" + code + "\\\") {"
            "  masterData {"
            "    abilities { gameID name }"
            "  }"
            "} } }\"}";

        std::string response = curlPost(API_URL, bearerToken, query);
        try {
            json j = json::parse(response);
            const auto& abilities =
                j["data"]["reportData"]["report"]["masterData"]["abilities"];
            for (const auto& ab : abilities)
                abilityMap[std::to_string(ab["gameID"].get<int>())] = ab["name"];
        } catch (const json::exception& e) {
            // Non-fatal — names will fall back to "Unknown"
            std::cerr << "[WCL] abilities parse error: " << e.what() << "\n";
        }
    }

    // ── 3. For each boss, page through all cast events ────────────────────────
    const std::string endStr = std::to_string(static_cast<long long>(fight.endTime));
    json allEvents = json::array();

    for (const auto& boss : bossActors) {
        const std::string sourceIDStr = std::to_string(boss.id);
        long long pageStart = fight.startTime;
        bool      morePages = true;

        while (morePages) {
            const std::string pageStartStr = std::to_string(pageStart);

            std::string query =
                "{\"query\":\"{ reportData { report(code: \\\"" + code + "\\\") {"
                "  events("
                "    startTime: "     + pageStartStr + ","
                "    endTime: "       + endStr       + ","
                "    sourceID: "      + sourceIDStr  + ","
                "    dataType: Casts,"
                "    hostilityType: Enemies"
                "  ) {"
                "    data"
                "    nextPageTimestamp"
                "  }"
                "} } }\"}";

            std::string response = curlPost(API_URL, bearerToken, query);
            try {
                json j          = json::parse(response);
                auto& eventsObj = j["data"]["reportData"]["report"]["events"];

                for (const auto& ev : eventsObj["data"]) {
                    json entry;
                    entry["timestamp"]    = ev.value("timestamp",    0LL);
                    entry["abilityGameID"] = ev.value("abilityGameID", 0);
                    entry["bossID"]       = boss.id;
                    entry["bossName"]     = boss.name;

                    std::string gameIDKey = std::to_string(ev.value("abilityGameID", 0));
                    entry["abilityName"]  = abilityMap.value(gameIDKey, "Unknown");

                    allEvents.push_back(entry);
                }

                if (eventsObj["nextPageTimestamp"].is_null())
                    morePages = false;
                else
                    pageStart = eventsObj["nextPageTimestamp"].get<long long>();
            } catch (const json::exception& e) {
                std::cerr << "[WCL] events page parse error (bossID=" << boss.id
                          << "): " << e.what() << "\n";
                break;
            }
        }
    }

    // ── 4. Sort all events by timestamp ascending ─────────────────────────────
    std::sort(allEvents.begin(), allEvents.end(),
              [](const json& a, const json& b) {
                  return a.value("timestamp", 0LL) < b.value("timestamp", 0LL);
              });

    // ── 5. Build final result ─────────────────────────────────────────────────
    json bossSummary = json::array();
    for (const auto& boss : bossActors)
        bossSummary.push_back({ {"id", boss.id}, {"name", boss.name} });

    json result;
    result["bosses"]       = bossSummary;
    result["castTimeline"] = allEvents;
    return result.dump(2);
}