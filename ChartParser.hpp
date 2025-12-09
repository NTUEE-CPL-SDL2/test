#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iostream>
#include <cstdint>
#include <cstddef>

// 按鍵音符結構（同學A使用）
struct KeyNoteData {
    std::size_t startFragment;
    std::size_t lane;
    int8_t holds;  // -1=TAP, >=1=持續fragments
};

// 滑鼠物件結構（同學B使用）
struct MouseNoteData {
    std::size_t startFragment;
    std::size_t lane;    // 軌道編號 (0-3)
    int type;            // 0=GREEN(拾取), 1=RED(躲避)
};

class ChartParser {
private:
    int bpm;
    int offset;
    int fragmentsPerBeat;
    std::string musicFile;
    std::vector<KeyNoteData> keyNotes;
    std::vector<MouseNoteData> mouseNotes;

    std::string trim(const std::string& str) {
        size_t first = str.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) return "";
        size_t last = str.find_last_not_of(" \t\r\n");
        return str.substr(first, last - first + 1);
    }
    
    void parseMetadata(const std::string& line) {
        if (line.find("&bpm=") == 0) {
            bpm = std::stoi(line.substr(5));
        } else if (line.find("&offset=") == 0) {
            offset = std::stoi(line.substr(8));
        } else if (line.find("&music=") == 0) {
            musicFile = line.substr(7);
        } else if (line.find("&fragments=") == 0) {
            fragmentsPerBeat = std::stoi(line.substr(11));
        }
    }
    
    // 不使用 seekg，改用返回下一行的方式
    std::string parseKeyNotes(std::ifstream& file) {
        std::string line;
        int currentDensity = 4;
        std::size_t currentFragment = 0;
        
        while (std::getline(file, line)) {
            line = trim(line);
            
            // 遇到下一個區塊標籤時，返回該標籤
            if (!line.empty() && line[0] == '&') {
                return line;
            }
            
            if (line.empty() || line[0] == '#') continue;
            
            if (line[0] == '{' && line.back() == '}') {
                currentDensity = std::stoi(line.substr(1, line.length() - 2));
                continue;
            }
            
            parseKeyNoteLine(line, currentDensity, currentFragment);
        }
        return "";
    }
    
    void parseKeyNoteLine(const std::string& line, int density, std::size_t& currentFragment) {
        std::size_t fragmentsPerGrid = (fragmentsPerBeat * 4) / density;
        std::stringstream ss(line);
        std::string token;
        
        while (std::getline(ss, token, ',')) {
            token = trim(token);
            if (!token.empty()) {
                parseKeyNoteToken(token, currentFragment, fragmentsPerGrid);
            }
            currentFragment += fragmentsPerGrid;
        }
    }
    
    void parseKeyNoteToken(const std::string& token, std::size_t fragment, std::size_t fragmentsPerGrid) {
        if (token.find('/') != std::string::npos) {
            std::stringstream ss(token);
            std::string noteStr;
            while (std::getline(ss, noteStr, '/')) {
                parseSingleKeyNote(noteStr, fragment, fragmentsPerGrid);
            }
        } else {
            parseSingleKeyNote(token, fragment, fragmentsPerGrid);
        }
    }
    
    void parseSingleKeyNote(const std::string& noteStr, std::size_t fragment, std::size_t fragmentsPerGrid) {
        if (noteStr.find("h[") != std::string::npos) {
            size_t hPos = noteStr.find('h');
            size_t bracketStart = noteStr.find('[');
            size_t bracketEnd = noteStr.find(']');
            
            int lane = std::stoi(noteStr.substr(0, hPos)) - 1;
            int grids = std::stoi(noteStr.substr(bracketStart + 1, bracketEnd - bracketStart - 1));
            int8_t holdFragments = static_cast<int8_t>(grids * fragmentsPerGrid);
            
            keyNotes.push_back({fragment, static_cast<std::size_t>(lane), holdFragments});
        } else {
            int lane = std::stoi(noteStr) - 1;
            keyNotes.push_back({fragment, static_cast<std::size_t>(lane), -1});
        }
    }
    
    // 不使用 seekg，改用返回下一行的方式
    std::string parseMouseNotes(std::ifstream& file) {
        std::cout << "[DEBUG] Entering parseMouseNotes()" << std::endl;
        std::string line;
        int currentDensity = 4;
        std::size_t currentFragment = 0;
        int lineCount = 0;
        
        while (std::getline(file, line)) {
            line = trim(line);
            lineCount++;
            std::cout << "[DEBUG] Mouse line " << lineCount << ": '" << line << "'" << std::endl;
            
            // 遇到下一個區塊標籤時，返回該標籤
            if (!line.empty() && line[0] == '&') {
                std::cout << "[DEBUG] Found next section tag: " << line << std::endl;
                return line;
            }
            
            if (line.empty() || line[0] == '#') {
                std::cout << "[DEBUG] Skipping empty/comment line" << std::endl;
                continue;
            }
            
            if (line[0] == '{' && line.back() == '}') {
                currentDensity = std::stoi(line.substr(1, line.length() - 2));
                std::cout << "[DEBUG] Density changed to: " << currentDensity << std::endl;
                continue;
            }
            
            std::cout << "[DEBUG] Parsing mouse note line" << std::endl;
            parseMouseNoteLine(line, currentDensity, currentFragment);
        }
        std::cout << "[DEBUG] Reached end of file, total mouse notes: " << mouseNotes.size() << std::endl;
        return "";
    }
    
    void parseMouseNoteLine(const std::string& line, int density, std::size_t& currentFragment) {
        std::size_t fragmentsPerGrid = (fragmentsPerBeat * 4) / density;
        std::stringstream ss(line);
        std::string token;
        
        while (std::getline(ss, token, ',')) {
            token = trim(token);
            if (!token.empty()) {
                std::cout << "[DEBUG] Mouse token: '" << token << "' at fragment " << currentFragment << std::endl;
                parseMouseNoteToken(token, currentFragment);
            }
            currentFragment += fragmentsPerGrid;
        }
    }
    
    void parseMouseNoteToken(const std::string& token, std::size_t fragment) {
        char type = token[0];
        
        if (type == 'G' || type == 'R') {
            // 格式: G1, G2, R3, R4
            int lane = std::stoi(token.substr(1)) - 1;  // 1-4 轉成 0-3
            int noteType = (type == 'G') ? 0 : 1;
            mouseNotes.push_back({fragment, static_cast<std::size_t>(lane), noteType});
            std::cout << "[DEBUG] Added mouse note: type=" << (noteType == 0 ? "GREEN" : "RED") 
                      << " lane=" << lane << " fragment=" << fragment << std::endl;
        } else {
            std::cout << "[DEBUG] Invalid mouse token type: " << type << std::endl;
        }
    }

public:
    ChartParser() : bpm(120), offset(0), fragmentsPerBeat(4) {}
    
    bool load(const std::string& filepath) {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            std::cerr << "[ERROR] Cannot open chart: " << filepath << std::endl;
            return false;
        }
        
        std::string line;
        std::string nextLine = "";
        
        while (true) {
            // 如果有緩存的下一行，使用它；否則從檔案讀取
            if (!nextLine.empty()) {
                line = nextLine;
                nextLine = "";
            } else {
                if (!std::getline(file, line)) break;
            }
            
            line = trim(line);
            std::cout << "[DEBUG] Main loop reading: '" << line << "'" << std::endl;
            
            if (line.empty() || line[0] == '#') continue;
            
            if (line.find("&bpm=") == 0 || line.find("&offset=") == 0 || 
                line.find("&music=") == 0 || line.find("&fragments=") == 0) {
                parseMetadata(line);
            } else if (line == "&keynotes=") {
                std::cout << "[DEBUG] Found &keynotes= tag" << std::endl;
                nextLine = parseKeyNotes(file);
            } else if (line == "&mousenotes=") {
                std::cout << "[DEBUG] Found &mousenotes= tag" << std::endl;
                nextLine = parseMouseNotes(file);
            }
        }
        
        file.close();
        
        std::sort(keyNotes.begin(), keyNotes.end(), 
            [](const KeyNoteData& a, const KeyNoteData& b) { return a.startFragment < b.startFragment; });
        std::sort(mouseNotes.begin(), mouseNotes.end(),
            [](const MouseNoteData& a, const MouseNoteData& b) { return a.startFragment < b.startFragment; });
        
        std::cout << "[OK] Chart loaded: " << filepath << std::endl;
        std::cout << "     BPM=" << bpm << ", Fragments/Beat=" << fragmentsPerBeat << std::endl;
        std::cout << "     Key notes=" << keyNotes.size() << ", Mouse notes=" << mouseNotes.size() << std::endl;
        
        return true;
    }
    
    const std::vector<KeyNoteData>& getKeyNotes() const { return keyNotes; }
    const std::vector<MouseNoteData>& getMouseNotes() const { return mouseNotes; }
    const std::string& getMusicFile() const { return musicFile; }
    int getBPM() const { return bpm; }
    int getOffset() const { return offset; }
    int getFragmentsPerBeat() const { return fragmentsPerBeat; }
    
    double getFragmentTime(std::size_t fragment) const {
        double beatDuration = 60000.0 / bpm;
        double fragmentDuration = beatDuration / fragmentsPerBeat;
        return offset + fragment * fragmentDuration;
    }
    
    void printChart() const {
        std::cout << "\n=== Chart Information ===" << std::endl;
        std::cout << "BPM: " << bpm << ", Offset: " << offset << " ms" << std::endl;
        std::cout << "Fragments per beat: " << fragmentsPerBeat << std::endl;
        std::cout << "Music file: " << musicFile << std::endl;
        
        std::cout << "\n=== Key Notes (" << keyNotes.size() << ") ===" << std::endl;
        if (!keyNotes.empty()) {
            std::cout << "Fragment\tLane\tHolds\tTime(ms)" << std::endl;
            for (size_t i = 0; i < std::min(size_t(5), keyNotes.size()); i++) {
                const auto& n = keyNotes[i];
                std::cout << n.startFragment << "\t\t" << n.lane << "\t";
                if (n.holds == -1) {
                    std::cout << "TAP";
                } else {
                    std::cout << (int)n.holds;
                }
                std::cout << "\t" << getFragmentTime(n.startFragment) << std::endl;
            }
            if (keyNotes.size() > 5) {
                std::cout << "... (showing first 5 of " << keyNotes.size() << ")" << std::endl;
            }
        }
        
        std::cout << "\n=== Mouse Notes (" << mouseNotes.size() << ") ===" << std::endl;
        if (!mouseNotes.empty()) {
            std::cout << "Fragment\tLane\tType\tTime(ms)" << std::endl;
            for (size_t i = 0; i < std::min(size_t(5), mouseNotes.size()); i++) {
                const auto& m = mouseNotes[i];
                std::cout << m.startFragment << "\t\t" << m.lane << "\t" 
                         << (m.type == 0 ? "GREEN" : "RED") << "\t" 
                         << getFragmentTime(m.startFragment) << std::endl;
            }
            if (mouseNotes.size() > 5) {
                std::cout << "... (showing first 5 of " << mouseNotes.size() << ")" << std::endl;
            }
        }
    }
};