#include "effects_ui.h"
namespace effectsui {
// Compatibility entry point. Verified effects are now drawn inline per hero by hero_info.h,
// not in a duplicate floating/debug window. showEffects/timedOnly still control those rows.
void Draw(const Frame&) {}
}
