#ifdef _WIN32
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00
#endif
#endif

#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include "operation.h"
#include "trie.h"
#include "httplib.h"

std::vector<std::string> splitString(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(str);
    while (std::getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

std::string toJsonArray(const std::vector<std::string>& vec) {
    std::string json = "[";
    for (size_t i = 0; i < vec.size(); ++i) {
        json += "\"" + vec[i] + "\"";
        if (i < vec.size() - 1) json += ",";
    }
    json += "]";
    return json;
}

int main() {
    Operation gameEngine;
    
    httplib::Server svr;
    
    // Load the global dictionary
    gameEngine.LoadDictionary("../data/full_vocab.txt");

    svr.set_mount_point("/", "../frontend");

    svr.Get("/api/generate", [&](const httplib::Request& req, httplib::Response& res) {
        std::vector<std::string> brokenWord = gameEngine.GenerateBrokenWord();
        std::string jsonStr = "{\"brokenWord\": " + toJsonArray(brokenWord) + "}";
        res.set_content(jsonStr, "application/json");
        res.set_header("Access-Control-Allow-Origin", "*");
    });

    svr.Get("/api/check", [&](const httplib::Request& req, httplib::Response& res) {
        if (!req.has_param("guess") || !req.has_param("broken")) {
            res.status = 400;
            res.set_content("{\"error\": \"Missing params\"}", "application/json");
            return;
        }

        std::string guess = req.get_param_value("guess");
        std::string brokenStr = req.get_param_value("broken");
        std::vector<std::string> brokenWord = splitString(brokenStr, ',');

        bool valid = gameEngine.checkWordValidity(guess, brokenWord);
        std::string jsonStr;
        if (valid) {
            jsonStr = "{\"valid\": true}";
        } else {
            std::string correctWord = gameEngine.fullWord(brokenWord);
            jsonStr = "{\"valid\": false, \"correctWord\": \"" + correctWord + "\"}";
        }
        res.set_content(jsonStr, "application/json");
        res.set_header("Access-Control-Allow-Origin", "*");
    });

    int port = 8080;
    std::cout << "Server starting at http://localhost:" << port << std::endl;
    svr.listen("0.0.0.0", port);
    
    return 0;
}