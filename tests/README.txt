Автономный тест без Windows/Dota:
g++ -std=c++17 tests/aegis_tracker_test.cpp -o aegis-test
./aegis-test
22 проверки. Это не тест работы DLL в игре.

COMBO18: tests/run-effects-tests.py now includes combo_core, combo-ui and
combo-integration, plus exact spell identity/phase assertions in game-memory tests.
Default uses ASan/UBSan. If sanitizer runtime is unavailable, explicitly use:
FDA_NO_SANITIZERS=1 python tests/run-effects-tests.py
This changes test instrumentation, not production guards.
Optional FDA_TEST_FILTER=combo_core,combo-ui,combo-integration,game-memory runs only
those groups; it is NOT a full regression run. No Windows/live Dota validation.
