FDA ESP20 — простой HP/MP/skills HUD + стабильная камера. Текущая версия.

ИЗМЕНЕНИЯ ПО ПОСЛЕДНЕМУ ОТЧЁТУ:
1. Верхние карточки: HP, мана, квадратные иконки скиллов и целые секунды оставшегося КД.
   Иконка затемняется на КД. Готовая — без цифры. Нет предметов, зарядов, точек уровней,
   Aegis footer, большого пустого второго ряда. Высота68 вместо112, defaultY54.
   До6 скиллов в одной строке, на узком экране квадраты уменьшаются, не переносятся столбцом.
   Неизвестная видимость: ?; подтверждённый туман: FOG. Скрытые current stats не выдумываются.
   Мана читается с проверкой успешного чтения и конечных значений: ? при unknown,
   -- при успешно прочитанных нулевых mana/maxMana (например герой без mana pool).
2. m_fCooldown больше не инвертируется глобальным full/ready голосованием других героев.
   Число КД берётся из конкретного предмета/скилла. Native поле ещё требует live проверки.
3. Матрица камеры читается дважды. Несовпадающий/невалидный кадр пропускается,
   старая матрица не переиспользуется. Режим камеры выбирается по её структуре/оси Z
   при инициализации и остаётся стабильным; НЕ переизбирается по положениям героев,
   скрытым координатам, количеству голосов или меняющейся camera translation.
   Есть mock regression: pan через origin, transpose/inverted init, fixed world point
   под текущей матрицей. Это не live проверка адреса матрицы/синхронизации GPU камеры.
4. Кружок хранит последнюю видимую WORLD позицию и каждый кадр проектируется текущей
   камерой. Камера движется -> экранная координата закономерно меняется, WORLD позиция
   остаётся прежней. Не приклеивается к пикселю экрана и не следует за hidden enemy pos.
   Вне безопасной области кружок целиком пропускается (не режется/не переносится на край).
5. World ESP рисуется только вне верхнего HUD/полосы карточек и нижней22% области экрана,
   консервативно зарезервированной под native HUD/миникарту. Clip rectangle применяется
   ко всем world marks. При FDA меню или потере foreground gameplay overlays скрыты;
   fog-memory продолжает обновляться. Верхние карточки отдельно clip внутри своей рамки.
   Это Present/ImGui 2D OVERLAY, не native scene pass: настоящей depth occlusion за
   деревьями/terrain НЕТ. Без depth buffer/native UI visibility нельзя честно обещать
   отсутствие пересечений со всеми Panorama окнами, scoreboard, shops/custom HUD sizes.
   Safe area фиксированная консервативная, не распознавание всех native UI rectangles.
6. Armlet ARM18 repair + девять COMBO18 basic spell profiles сохранены, не удалены.
   Lion, Lina, Sven, Vengeful Spirit, Ogre Magi, Zeus, Queen of Pain, Luna, Dragon Knight.
   Остальные возможности остаются как ESP19. Это не все127 и не полный порт Umbrella.

ПОЧЕМУ ESP ПОЯВИЛСЯ ПОЗЖЕ:
По одному screenshot невозможно доказать конкретную причину. Scanner/RTTI/local binding
и guarded team visibility должны сначала дать валидный snapshot; таймер, который заставляет
ждать перед показом, здесь не добавлен. Не отменялись guards и не форсировалась видимость.
ESP19 team bit offset/index остаётся кандидатом: offsetsLiveVerified=0. Позднее появление
не доказывает, что все guards/mapping правильные. Для проверки нужен Copy ESP diagnostics
сразу после входа (когда отсутствует) и после появления: frameGate, localFrame,
teamDataCandidates, npcVision (own + visible enemy), matrixPlausible/mode и M0..M3.
При продолжающемся прыжке: два снимка M0..M3 при неподвижном и перемещённом экране.

КАК ОБНОВИТЬ:
Весь FDA-ESP20 в НОВУЮ папку проекта -> build.bat на Windows MSVC x64/Windows SDK.
Не использовать предыдущую DLL после ошибки сборки. В меню и Copy ESP должен быть ESP20.
Старые README/QA исторические, текущие README-ESP20.txt и QA-ESP20.txt.
Профиль мигрируется на uiVersion13: topY54 + skills enabled; затем пользовательский Y сохраняется.
Иконки/шрифты в assets должны быть доступны рядом с build DLL или в parent assets.
No prebuilt DLL: Linux sandbox не предоставляет Windows/live Dota/GPU тест.
Сначала demo: видимый enemy -> FOW -> camera pan -> reappearance. Затем проверить КД
одного заклинания и его возвращение к готовности. Автоматизация сохранила fail-closed guards.

QA:
Portable/mock tests без ASan/UBSan; текущий log в QA-ESP20.txt.
Actual C++ Top/World/Fog synthetic render на1920/1280/hidden; вручную осмотренные PNG в tests/.
Это synthetic fixtures, не live Dota capture и не доказательство live offsets.
Установленные файлы игры не читались/не менялись, upstream код не исполнялся.
