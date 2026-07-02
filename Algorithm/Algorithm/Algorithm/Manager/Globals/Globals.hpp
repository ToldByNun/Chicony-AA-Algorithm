#pragma once

class ClassManager;
class Overlay;
class Visuals;

struct Globals {
    ClassManager* classManager = nullptr;
    Overlay* overlay = nullptr;
    Visuals* visuals = nullptr;
};

extern Globals globals;
