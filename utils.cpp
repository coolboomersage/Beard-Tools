#include "utils.h"

#include <fstream>
#include <filesystem>
#include <stdexcept>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <unistd.h>
    #include <limits.h>
#endif

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

std::vector<std::string> fetchKeys(const std::string& filePath) {
    std::ifstream file(filePath);

    if (!file.is_open()) {
        std::ofstream newFile(filePath);
        if (!newFile.is_open())
            throw std::runtime_error("Failed to create file: " + filePath);

        newFile << "ClientID=your_client_id_here\n";
        newFile << "ClientSecret=your_client_secret_here\n";
        newFile.close();
        throw std::runtime_error(
            "File created at: " + filePath +
            ". Please populate the ClientID and ClientSecret fields.");
    }

    std::string line;
    std::string clientID, clientSecret;
    const std::string idPrefix     = "ClientID=";
    const std::string secretPrefix = "ClientSecret=";

    while (std::getline(file, line)) {
        size_t pos;
        if ((pos = line.find(idPrefix)) != std::string::npos)
            clientID = line.substr(pos + idPrefix.length());
        else if ((pos = line.find(secretPrefix)) != std::string::npos)
            clientSecret = line.substr(pos + secretPrefix.length());
    }

    if (clientID.empty())
        throw std::runtime_error("ClientID not found in file: " + filePath);
    if (clientSecret.empty())
        throw std::runtime_error("ClientSecret not found in file: " + filePath);

    return { clientID, clientSecret };
}

void appendToken(const std::string& filePath, const std::string& token) {
    std::ofstream file(filePath, std::ios::app);
    if (!file.is_open())
        throw std::runtime_error("Failed to open file: " + filePath);
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