#pragma once

namespace Settings {
struct Values {
    bool debugLogging = false;
};

const Values& Get();
void Reload();
}
