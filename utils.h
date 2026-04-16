#pragma once

#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// Returns the directory containing the running executable.
// ---------------------------------------------------------------------------
std::string getExecutableDirectory();

// Path to the API key file, evaluated each call (follows the executable).
#define KEY_FILE_DIRECTORY (std::string(getExecutableDirectory()) + "/APIkey.txt")

// ---------------------------------------------------------------------------
// Reads ClientID and ClientSecret from the key file.
// Creates the file with placeholder values if it does not exist.
// Throws std::runtime_error on any failure.
// Returns { clientID, clientSecret }.
// ---------------------------------------------------------------------------
std::vector<std::string> fetchKeys(const std::string& filePath);

// ---------------------------------------------------------------------------
// Appends a token= line to the key file so subsequent runs skip OAuth.
// ---------------------------------------------------------------------------
void appendToken(const std::string& filePath, const std::string& token);

// ---------------------------------------------------------------------------
// Extracts the bare report code from a full WCL URL or returns the input
// unchanged if no "/reports/" marker is found.
// ---------------------------------------------------------------------------
std::string ExtractReportCode(const std::string& input);