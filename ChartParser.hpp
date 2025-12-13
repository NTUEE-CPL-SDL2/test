#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iostream>
#include <cstdint>
#include <cstddef>

#include "include/vector.hpp"
#include "include/qsort.hpp"

#include "KeyNoteData.hpp"

// 滑鼠物件結構（同學B使用）
struct MouseNoteData {
    std::size_t startFragment;
    std::size_t lane;       // 軌道編號 (0-3)
    int type;               // 0=GREEN(拾取), 1=RED(躲避)
};

class ChartParser {
private:
    int bpm;
    int offset;
    int fragmentsPerBeat;
    std::string musicFile;
    mystd::vector<KeyNoteData>& keyNotes;
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
    
    std::string parseKeyNotes(std::ifstream& file) {
        std::string line;
        int currentDensity = 4;
        std::size_t currentFragment = 0;
        
        while (std::getline(file, line)) {
            line = trim(line);
            
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
    
    std::string parseMouseNotes(std::ifstream& file) {
        std::string line;
        int currentDensity = 4;
        std::size_t currentFragment = 0;
        
        while (std::getline(file, line)) {
            line = trim(line);
            
            if (!line.empty() && line[0] == '&') {
                return line;
            }
            
            if (line.empty() || line[0] == '#') continue;
            
            if (line[0] == '{' && line.back() == '}') {
                currentDensity = std::stoi(line.substr(1, line.length() - 2));
                continue;
            }
            
            parseMouseNoteLine(line, currentDensity, currentFragment);
        }
        return "";
    }
    
    void parseMouseNoteLine(const std::string& line, int density, std::size_t& currentFragment) {
        std::size_t fragmentsPerGrid = (fragmentsPerBeat * 4) / density;
        std::stringstream ss(line);
        std::string token;
        
        while (std::getline(ss, token, ',')) {
            token = trim(token);
            if (!token.empty()) {
                parseMouseNoteToken(token, currentFragment);
            }
            currentFragment += fragmentsPerGrid;
        }
    }
    
    // 修正版：支援和弦 (G1/R2 表示同時出現綠色1和紅色2)
    void parseMouseNoteToken(const std::string& token, std::size_t fragment) {
        // 檢查是否為和弦
        if (token.find('/') != std::string::npos) {
            std::stringstream ss(token);
            std::string singleNoteStr;
            
            // 逐一解析和弦中的每個物件
            while (std::getline(ss, singleNoteStr, '/')) {
                singleNoteStr = trim(singleNoteStr);  // 移除空白
                if (!singleNoteStr.empty()) {
                    parseSingleMouseObject(singleNoteStr, fragment);
                }
            }
        } else {
            // 單一物件
            parseSingleMouseObject(token, fragment);
        }
    }
    
    // 新增：解析單一 Mouse Object
    void parseSingleMouseObject(const std::string& noteStr, std::size_t fragment) {
        if (noteStr.length() >= 2) {
            char type = noteStr[0];
            
            if (type == 'G' || type == 'R') {
                try {
                    int lane = std::stoi(noteStr.substr(1)) - 1;  // 1-4 轉成 0-3
                    int noteType = (type == 'G') ? 0 : 1;
                    
                    if (lane >= 0 && lane < 4) {  // 假設 LANES = 4
                        mouseNotes.push_back({fragment, static_cast<std::size_t>(lane), noteType});
                    } else {
                        std::cerr << "[WARNING] Mouse Note lane out of bounds: " << (lane + 1) 
                                  << " at fragment " << fragment << std::endl;
                    }
                } catch (const std::invalid_argument& e) {
                    std::cerr << "[WARNING] Invalid Mouse Note format: " << noteStr 
                              << " at fragment " << fragment << std::endl;
                } catch (const std::out_of_range& e) {
                    std::cerr << "[WARNING] Mouse Note lane number too large: " << noteStr 
                              << " at fragment " << fragment << std::endl;
                }
            }
        }
    }

public:
    ChartParser(mystd::vector<KeyNoteData>& keyNotes_) 
        : bpm(120), offset(0), fragmentsPerBeat(4), keyNotes(keyNotes_) {}
    
    bool load(const std::string& filepath) {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            std::cerr << "[ERROR] Cannot open chart: " << filepath << std::endl;
            return false;
        }
        
        std::string line;
        std::string nextLine = "";
        
        while (true) {
            if (!nextLine.empty()) {
                line = nextLine;
                nextLine = "";
            } else {
                if (!std::getline(file, line)) break;
            }
            
            line = trim(line);
            
            if (line.empty() || line[0] == '#') continue;
            
            if (line.find("&bpm=") == 0 || line.find("&offset=") == 0 || 
                line.find("&music=") == 0 || line.find("&fragments=") == 0) {
                parseMetadata(line);
            } else if (line == "&keynotes=") {
                nextLine = parseKeyNotes(file);
            } else if (line == "&mousenotes=") {
                nextLine = parseMouseNotes(file);
            }
        }
        
        file.close();
        
        qsort(keyNotes.begin(), keyNotes.end(), 
            [](const KeyNoteData& a, const KeyNoteData& b) { 
                return a.startFragment < b.startFragment; 
            });
        qsort(mouseNotes.begin(), mouseNotes.end(),
            [](const MouseNoteData& a, const MouseNoteData& b) { 
                return a.startFragment < b.startFragment; 
            });
        
        std::cout << "[OK] Chart loaded: " << filepath << std::endl;
        std::cout << "      BPM=" << bpm << ", Fragments/Beat=" << fragmentsPerBeat << std::endl;
        std::cout << "      Key notes=" << keyNotes.size() << ", Mouse notes=" << mouseNotes.size() << std::endl;
        
        return true;
    }
    
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
