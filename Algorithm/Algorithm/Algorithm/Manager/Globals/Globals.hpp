#pragma once

class ClassManager;
class Overlay;
class Assist;
class Visuals;

struct Globals {
    ClassManager* classManager = nullptr;
    Overlay* overlay = nullptr;
    Assist* assist = nullptr;
    Visuals* visuals = nullptr;
};

extern Globals globals;
