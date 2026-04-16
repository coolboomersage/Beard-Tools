#pragma once

#include <string>
#include "types.h"

// ---------------------------------------------------------------------------
// Retrieves a bearer token for the WCL API.
// Checks the key file for a cached token first; falls back to OAuth if absent.
// ---------------------------------------------------------------------------
std::string FetchOAuthToken(const std::string& clientId,
                            const std::string& clientSecret);

// ---------------------------------------------------------------------------
// Fetches top-level report metadata (title, owner, fights, player lists)
// and returns the raw JSON response string.
// ---------------------------------------------------------------------------
std::string FetchReportData(const std::string& bearerToken,
                            const std::string& reportCode);

// ---------------------------------------------------------------------------
// Fetches the table (gear/stats) and full paginated cast-event list for a
// single player in a single fight.  Returns a JSON object with two keys:
//   "table"  — the WCL table response
//   "events" — merged array of all cast-event pages
// ---------------------------------------------------------------------------
std::string fetchPersonalFightData(const Fight&       fight,
                                   int                playerId,
                                   const std::string& reportCode,
                                   const std::string& bearerToken);

// ---------------------------------------------------------------------------
// Fetches all boss cast events for a fight and returns a JSON object:
//   "bosses"       — array of { id, name }
//   "castTimeline" — flat, time-sorted array of cast events
// ---------------------------------------------------------------------------
std::string fetchBossCastTimeline(const Fight&       fight,
                                  const std::string& reportCode,
                                  const std::string& bearerToken);