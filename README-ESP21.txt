FDA ESP21 — current merged source: own-hero Armlet + combo usability + Umbrella GUI adaptation + manually verified visual pointer paths.

ЧТО ПОКАЗЫВАЕТ ТВОЙ ESP19 ЛОГ:
assigned=0x80A3 совпадает с Lina/player0; other Huskar handles не назначены твоему controller.
Only your own Huskar — старое ограничение модуля, до чтения/нажатия предмета. inputCalls=0.
На последних закрытых кадрах Lina HP2386 выше threshold350. Старые low-HP события были OFF.
Такой лог не доказывает сбой SendInput или чтения m_bToggleState: item gate не достигнут.
В ESP21 ограничение по имени Huskar убрано. Не выбирается чужой Huskar вместо assigned hero.
Нужен настоящий Armlet у СОБСТВЕННОГО героя в выбранном активном слоте1..6.

ARMLET:
- Любой собственный assigned герой, не illusion; валидная singleton selection, owner handle,
  inventory/modifiers/item serial/readiness/state gates, меню/focus/pause guards сохранены.
- Low HP: OFF -> ON; ON -> OFF, observe actual OFF, then ON. m_bToggleState/raw and
  verified unholy_strength effect используются; ключ предмета отправляется через SendInput.
  Не write-to-Panorama и не поддельный toggle флаг. Core ARM18 сохранён.
- Input lane только при low HP с подтверждённым Armlet в нужном слоте или pending cycle.
  Enabled alone, высокий HP или отсутствие предмета НЕ забирают очередь прокаста.
- Перед вводом дополнительно повторяются assigned/selected/hero/item-generation checks.
- Это пороговая автоматика, НЕ full incoming attack/projectile/DoT predictor, не гарантирует
  выживание. Узнанный опасный DoT блокирует новый OFF; уже начатый ON recovery не отменяет.
- Настройка: Armlet/own hero (ПКМ настройки), включить; слот1..6, настоящая клавиша слота;
  проверить вручную в демо, подтвердить; закрыть меню и выбрать только своего героя.
  Бинд включения необязателен. Даже без него enable в меню работает. При смене героя/матча
  подтверждение native setup сбрасывается. Если цикл fault/Armlet may be OFF — проверить
  вручную, reset и снова подтвердить, а не вслепую нажимать.

КАК ЗАПУСТИТЬ ПРОКАСТ LINA:
1. Прокасты / Помощник -> ПКМ Lina / basic burst -> Включить базовый прокаст.
2. Бинд удержания Space (кнопка «Назначить Space» при unset) или другая отдельная клавиша.
   Бинд не должен совпадать с Q/W/R, menu/unload. Не держать Shift/Ctrl/Alt/кнопки мыши.
3. Native spell keys совпадают с ТВОИМИ Dota quickcast-on-keydown биндами. По умолчанию:
   Light Strike Array=W; Dragon Slave=Q; Laguna Blade=R. Фактический порядок W -> Q -> R.
   Не угадывается пользовательская раскладка; проверь вручную и включи подтверждение.
4. Прокачать эти три заклинания, дождаться готовности, обеспечить ману для всей серии.
   Выбрать только собственную Lina; видимый реальный враг в500 range, курсор над ним.
5. Закрыть FDA меню/чат/консоль, focus Dota -> удерживать Space до завершения серии.
   Одна серия на удержание; после конца/ошибки отпустить и удержать снова.
6. E у Lina пассивная — её не «прожать». Встроенные планы содержат2..3 активных спелла,
   НЕ все клавиши подряд, НЕ items/Blink/channel/invoke/vector logic и НЕ all127 profiles.
   Другие профили: Lion, Sven, Vengeful Spirit, Ogre Magi, Zeus, QoP, Luna, Dragon Knight.
7. Последующий input только после cooldown ACK конкретного предыдущего спелла. После ACK
   теперь допускается до2s ожидания окончания прежней phase/active ability без немедленного
   fault. Не бесконечное ожидание/channel skipping. Цель в fog/невалидные данные блокируют.
8. Если серия остановилась: после отпускания открыть профиль -> «Копировать статус прокаста».
   Последний outcome сохраняется после Release/reset и содержит профиль/шаг/причину.
   Это не гарантия попадания point-target спеллов или убийства.

GUI/РЕНДЕР:
- User MEGA UmbrellaMenu.rar скачан, regular source files распакованы; EXE НЕ запускался.
  GUI21-SOURCE-REVIEW.json хранит hash/provenance. MEGA file MAC не проверялся; HTTPS
  download + RAR extraction/CRC не доказательство авторства. Никакой чужой код не запускался.
- Геометрия/palette/toggle animation/soft shadow из образца адаптированы под реальные FDA
  controls. Это НЕ wholesale запуск standalone replica с dummy functions. Base ImGui
  v1.92.8 WIP/backends/свои fonts retained; не подменял совместимый runtime старой библиотекой.
- Тёмный rail/sidebar/content, purple default accent, мягкие обводки/тени, анимированные
  toggle knobs. Сохраняются карточки HP/MP + skill cooldowns без верхних предметов/level dots.
- Existing runtime callbacks/configs/combo/diagnostics preserved. GUI — адаптация, не полный
  1:1 clone всех context popups, standalone settings window, fonts, backdrop effects.
- World lens convention stable under camera pan, frozen last-seen WORLD marker + safe clip
  preserved from ESP20. Present 2D overlay всё ещё НЕ depth-occluded за деревьями/terrain.
- Preview PNGs: source-extracted shell/widgets with fixture detail panels (render/Armlet),
  real combo UI and real visual-control detail block; NOT full live Windows menu/game capture.

ZOOM + WEATHER (АДРЕСА НАДО ПРОВЕРИТЬ НА ТВОЁМ КЛИЕНТЕ):
Прочее -> Camera/verified paths; Weather/verified paths. Реальный typed reader/writer,
но DEFAULT OFF, paths empty, demo confirmation false. Не «работает из коробки».
Нужны client.dll RVA/pointer chains для dota_camera_distance(float), r_farz(float),
fog_enable(byte), cl_weather(int32). Старый пример0x10 и old VMT layouts НЕ используются.
Формат input: HEX RVA, HEX offset, HEX offset — максимум3 dereferences. Без offsets direct
client.dll+RVA. Каждый offset: read uintptr at current address, then add offset.
Root внутри current client.dll image; stable pointer/value double reads, value-range checks.
Проверки диапазона НЕ доказывают, что адрес — нужный ConVar. Используй только в своей
приватной demo проверке: проверь эффект, тип значения и повторное разрешение после restart.
Checkbox demo confirmation — ТВОЁ подтверждение, не автоматическое доказательство demo mode.
Zoom800..2400 -> distance float; farz=2*distance; fog byte0. Weather ID0..9 int32.
Changed client/context/path, unknown reads, overlapping zoom addresses, implausible values
или game/user value mutation block new writes. Group write failure attempts rollback.
Disable/unload attempts restore только при same context/address and owned last written value;
foreign values не overwrites; changed context doesn't write stale addresses and can leave
previous visual values unchanged. Restoration не гарантирована после exit/error/context loss.
Настройки/paths не сохраняются специально: нет auto-writing после restart. Save остальных
FDA profiles не сохраняет эти visual paths. Need re-enter/reconfirm in every new demo session.
Не делает ConVar spoofing, не removes cheat flags, не патчит installed binaries/security.
Текущие live адреса отсутствуют; нужен твой проверенный offsets.json/цепочки для сверки.

PUBLIC SOURCE REVIEW:
McDota (Linux/GPL3 historical SDK/GUI), Aspirin493/dota-cheat (MIT), official Dear ImGui
просмотрены read-only по pinned commit hashes. Stars/forks — не отзывы и не подтверждение
качества/совместимости текущей Dota; проверенных «отличных отзывов» не установлено.
Old ConVar virtual layout не использован как current Windows ABI. Spoofing/stealth/anti-cheat
code не перенесён. См. SOURCE-REVIEW-ESP21.json и references/esp21-public-review.
User-provided GUI demo ownership/license вне встроенного ImGui не установлены; private
adaptation reference, не обещание прав на публичное распространение upstream demo.

ОБНОВЛЕНИЕ:
Распаковать ВЕСЬ FDA-ESP21 в НОВУЮ папку проекта -> README-ESP21 -> build.bat на Windows
MSVC x64 + Windows SDK. Marker меню/Copy ESP/Armlet должен быть ESP21, не ESP19.
Prebuilt DLL нет. Ошибка сборки -> не использовать старую DLL как будто обновилась.
26 portable/mock groups plus final targeted ownership/UI and visual adapter tests;
manual synthetic GUI/HUD previews. Windows ABI/GPU/live Dota/input/ConVar effects unverified.
QA-ESP21.txt указывает границы каждого теста. Older README/QA files are historical.
