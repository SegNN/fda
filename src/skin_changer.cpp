#include "skin_changer.h"

// Temporary shutdown build. No Steam API, hooks, account-cache projection,
// profile reads/writes or access to game files.
namespace skins {
const cosmetic::Definition* Catalog(size_t& count) { count = 0; return nullptr; }
void Poll() {}
Status GetStatus() {
    Status s;
    s.message = "Cosmetics disabled: no Steam GC hook or inventory projection.";
    return s;
}
bool SetEnabled(bool on) { return !on; }
bool Refresh() { return false; }
bool Select(uint32_t, uint32_t, uint32_t, uint32_t) { return false; }
bool Reset() { return true; }
bool CanUnload() { return true; }
void NotifyUnloadBlocked() {}
void Shutdown() {}
}
