#include "../src/skin_changer.h"
#include <cassert>
int main() {
    for (int i=0;i<100;++i) skins::Poll();
    auto s=skins::GetStatus();
    assert(s.message == "Cosmetics disabled: no Steam GC hook or inventory projection.");
    assert(!s.connected&&!s.enabled&&!s.cacheReady&&!s.pending&&!s.definitions);
    assert(!skins::SetEnabled(true)); assert(skins::SetEnabled(false));
    assert(!skins::Refresh()); assert(!skins::Select(1,1,1,0));
    size_t n=999;assert(skins::Catalog(n)==nullptr&&n==0);
    assert(skins::Reset()&&skins::CanUnload());skins::Shutdown();
}
