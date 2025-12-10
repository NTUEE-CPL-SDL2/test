#pragma once

struct KeyNoteData {
    std::size_t startFragment;
    std::size_t lane;
    int8_t holds;  // -1=TAP, >=1=持續fragments
};