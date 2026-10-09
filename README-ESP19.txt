FDA ESP19 — ОДИН ОБЪЕДИНЁННЫЙ ПРОЕКТ: COMBO18 + ARM18 + ESP/HUD исправления

ARM18 ранее был выпущен на старой базе ESP17 и НЕ содержал 9 COMBO18 profiles.
В ESP19 это исправлено: девять базовых spell sequences И исправленный Armlet вместе.
Это полный ИСХОДНЫЙ комплект, а не проверенная DLL. Windows/live Dota не проверены.
Старые README/QA — исторические; текущий файл README-ESP19.txt.

ГЕРОЙСКИЕ МОДУЛИ — «Помощники»:
Huskar / Armlet (с исправлениями ARM18) + 9 экспериментальных basic profiles:
Lion, Lina, Sven, Vengeful Spirit, Ogre Magi, Zeus, Queen of Pain, Luna, Dragon Knight.
Бинды, enable и native spell keys сохраняются; quickcast confirmation не сохраняется.
Реальный C++ executor, не пустые пункты. См. README-COMBO18.txt по последовательностям.
Это НЕ полный порт скриптов Umbrella/127 героев: нет item/Blink/vector/clone/invoke logic,
hit prediction, talent/facet strategy. Количество basic profiles всё ещё 9, не все герои.
Armlet владеет input lane на своём Huskar или при незавершённом цикле. Включённый
Huskar-модуль теперь НЕ блокирует basic combo другого героя.
Сначала проверить native spell keys в ДЕМО: quickcast на нажатие, отдельный hold bind.

ПОЧЕМУ В ARM18 БЫЛ ВИДЕН ТОЛЬКО СВОЙ ГЕРОЙ:
Твой отчёт показывает два прочитанных/проецируемых героя, но visionMask=0 у обоих,
включая явно видимого врага на скриншоте. Поэтому общая маска модели 0x788 не является
достаточным источником native NPC visibility на этом клиенте. По ней ранее скрывался враг
и не сохранялась его последняя видимая позиция. Матрицу и существующий entity parser
бесконтрольно не менял: они в твоём отчёте уже дают подходящие координаты обоих героев.

НОВЫЙ ЧИТАТЕЛЬ ВИДИМОСТИ (КАНДИДАТ, НЕ LIVE-VERIFIED):
- Существующий entity scan находит ровно один exact C_DOTA_DataRadiant и DataDire отдельно.
  Для своего localTeam выбирается соответствующий объект, не enemy/spectator data.
- Командная m_bNPCVisibleState, offset 0xE58, uint64[256] = 16384 NPC bits.
  Интепретация-кандидат: handle index = fullHandle & 0x3FFF; word=index/64, bit=index%64.
- Exact class, cached full data handle/serial/backlinks, local flag/assigned hero,
  exact hero/target generation, self NPC bit, bounded/stable word reads и повторные проверки.
  Duplicate candidates, spectator/wrong team, changed serial, missing self bit, torn reads
  -> неизвестная видимость, НЕ разрешение действий или live ESP.
- Задача reader — читать, а не устанавливать/форсировать видимость.
- Нулевая общая model mask теперь НЕ считается сама по себе подтверждённым туманом.
  НеZero legacy model mask остаётся явно вторичным fallback; если нет надёжного сигнала,
  enemy world overlay не выдумывается. Dormancy только диагностическая, НЕ равна FOW.
- Один local visibility decision используется world ESP, top HUD и basic/KS helpers.
  При потере видимости цель прокаста блокируется, её скрытая позиция не используется.
- STATIC offsets/mapping ещё требуют сверки на твоём клиенте. Нельзя утверждать,
  что этот читатель уже работает в живой Dota. Есть offsetsLiveVerified=0 в отчёте.
  Требование положительного собственного NPC bit может не выполняться при смерти/неполном
  наборе сетевых данных; в этом случае reader намеренно возвращает неизвестный результат.

КРУЖОК ПОСЛЕ УХОДА В ТУМАН:
Виден -> запоминается позиция и портрет по полному handle. Виден -> скрыт -> 10s ring
НА ПОСЛЕДНЕЙ ВИДИМОЙ позиции; hidden snapshot coordinates НЕ обновляют marker.
Возвращение в обзор немедленно убирает marker; конец10s убирает; новый матч/own context reset
очищает память. Нужен хотя бы один подтверждённый видимый кадр до ухода. Никогда не видели
или visibility неизвестна — не рисуется выдуманная позиция. Смерть подтверждается только
видимым наблюдением, не неизвестным hidden HP0. За пределами viewport marker не рисуется.
Timer использует game clock и не истекает на паузе.

КОМПАКТНЫЙ HUD:
- Старый длинный столбец под narrow pixel portrait anchors заменён горизонтальными карточками.
- Две верхние группы Radiant/Dire, до5 игроков в каждой, стабильные player slots.
  HP/MP -> горизонтальный ряд спеллов -> горизонтальный ряд активных предметов.
  Поддерживаются одинаковые герои разных команд; иллюзии не занимают отдельную карточку.
- 1920px: иконки24px, 6 в ряд, height112 вместо длинной вертикальной цепочки.
  Меньший экран: иконки18px, +N явно означает не поместившиеся иконки. Не растёт столбец.
  Неизвестно: VISION ?; нет кадра: NO SNAPSHOT; подтверждённый туман: FOG.
  В этих состояниях нет текущих enemy HP/MP/skills/items. Портрет — статическая metadata.
- Pixel portrait capture больше не нужен для этой панели; тяжёлый ROI search не запускается.
- World abilities/items/effects — горизонтально НАД HP-баром. Native Dota HP-бар не рисуется
  повторно, только прежнее число HP. Если место над героем пересекает фиксированный top HUD,
  world rows пропускаются, а информация остаётся в верхней карточке; не телепортируется к
  другому герою и не перекрывает top cards.
- Панель настраивается: TopPanel enable, top abilities/portraits, Compact panel Y.
  Размер world icons остаётся отдельным прежним параметром. Старые topSlot/topGap INI поля
  сохранены для совместимости, но не задают narrow portrait-column layout.

КАК ОБНОВИТЬ:
1. ВЕСЬ FDA-ESP19 архив в НОВУЮ папку ПРОЕКТА, не в установленную папку Dota.
2. README-ESP19.txt -> build.bat (Windows, MSVC x64 Desktop development with C++ / Windows SDK).
   Никакой prebuilt DLL нет; не использовать DLL от прошлой сборки при ошибке компиляции.
3. В меню/диагностике должен быть ESP19. Помощники: 10 пунктов = Armlet + 9 basic modules.
4. В ДЕМО сначала проверь видимого врага, затем его уход/возврат. Если overlay/ring не работает,
   скопировать Copy ESP diagnostics. Для новой проблемы нужны только teamDataCandidates и
   npcVision строки для своего героя и противника (один снимок «виден», второй «ушёл»).
   tableRead=1 и visible=1 должны быть у видимого врага, visible=0 после ухода.
   probeMask bits1/2 означают только сырой self/target word read, НЕ подтверждение видимости.
   word/selfWord при tableRead=0 — непроверенные diagnostic candidates, не игровое решение.
5. Чтобы запускать basic combo: настроить ключи, подтвердить после проверки, закрыть чат,
   консоль и FDA меню, выбрать только своего героя, навести курсор на видимую цель и держать
   отдельный бинд. Один проход на удержание. Это не гарантия попадания/убийства.

ПРОВЕРКИ/ОГРАНИЧЕНИЯ:
Linux portable/mock regression + actual C++ Top/World/Fog render fixtures.
25 групп mock/portable, без ASan/UBSan (runtime отсутствует). Изображения проверены вручную
на1920/1280 и в fog/unknown/missing состояниях. QA-ESP19.txt содержит логи и границы QA.
Это не Windows ABI/GPU/live match проверка. Скриншоты tests/ESP19-* — synthetic fixtures,
НЕ скриншоты твоей Dota и не доказательство live offsets.
Armlet fixes ARM18 сохранены: immediate ready observed OFF -> ON, verified active effect,
no false NET HP gain requirement. Полный incoming attack/projectile/DoT predictor отсутствует.
Файлы установленной игры не читались/не менялись, античит/авторизация не обходились,
сторонние Lua/Rust не запускались, GitHub не обновлялся. Лицензии reference files сохранены.
