Standalone tests: no Dota, Steam or Windows required.
From project root on Linux with g++:
  g++ -std=c++17 -O2 tests/cosmetic_core_test.cpp -o /tmp/cosmetic-core-test
  /tmp/cosmetic-core-test
  g++ -std=c++17 -O2 -Itests/winshim tests/cosmetic_adapter_test.cpp src/skin_changer.cpp -o /tmp/cosmetic-adapter-test
  /tmp/cosmetic-adapter-test

Optional: -fsanitize=address,undefined (requires the matching runtime libraries).
The adapter test uses mock functions and a mock three-method Steam interface.
It does NOT verify Steam behavior, Windows ABI, GC callbacks, cosmetic models,
write-to-disk behavior of Windows profile APIs, or absence of bans.
Use the real Windows SDK for production. NEVER add tests/winshim to build.bat.
