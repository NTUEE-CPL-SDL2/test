@echo off

echo === Building RhythmQuest ===

g++ RhythmQuest.cpp -o RhythmQuest.exe ^
-I. ^
-IC:\SDL2\include ^
-IC:\SDL2\include\SDL2 ^
-LC:\SDL2\lib ^
-lmingw32 -lSDL2main -lSDL2 -lSDL2_mixer -lSDL2_image -lSDL2_ttf ^
-std=c++20

echo === Build Finished ===
pause
