# Найденные исходники прокастов и помощников

## Реально включено в комплект — только справочные исходники

- Eroica-cpp/dota2scripts, MIT, commit e00695943eaf866181a6dbd8279bd0ade0b59ddb:
  43 Lua-файла. Есть Invoker, Tinker, EarthSpirit, Axe, Legion, ShadowFiend, Armlet,
  Rubick, Morphling и другие. API Heroes.GetLocal, NPC.GetAbility, Ability.Cast*, Menu.*
  предоставляется внешним окружением, а не Lua и не FDA. Последний commit 2021.
- forest0xia/dota2bot-OpenHyperAI, MIT, commit cb814c6c8dc51ed08045d6efd9f4a48147992711:
  128 BotLib/hero_*.lua. Есть соответствие всем 127 героям текущего FDA-каталога.
  Функции SkillsComplement есть как прямые X-методы или экспортированные локальные функции
  (например Io). Это логика БОТОВ через GetBot / Action_UseAbility* / J helper library.
  Наличие функций не означает проверенные прокасты для управляемого человеком героя.
- zfael/dota2-scripts, MIT, commit 0c35ea8620c8f5979336bd278f2f95b0e776a70b:
  выбранные Rust-модули героев и README. Другой GSI/Rust runtime с ActionExecutor и моделями.
  Не готовая C++ библиотека FDA.

Сами файлы: references/combo-sources/snapshots/.
Точное соответствие каждого героя: COMBO-SOURCE-COVERAGE.json.
Происхождение/контрольные суммы каждого файла: references/combo-sources/fetched-files.json.
MIT copyright notices и комментарии исходных авторов сохранены.
Снимки неполные относительно upstream проектов, сторонние зависимости не установлены.

## Что НЕ объявляется готовым

Полные сторонние алгоритмы этих героев НЕ перенесены. В COMBO18 подключены только
9 собственных ограниченных native spell sequences; см. README-COMBO18.txt.
Новые Lua/Rust файлы не участвуют в build.bat и не исполняются.
Добавление исходника в references != реализация рабочего модуля.
Реверс Octarine сам по себе не предоставил исполняемый Lua/C++ SDK и ABI всех приказов.

Для переноса нужны:
1. Native adapter: local hero, entity handles/serials, target/location/no-target orders,
   queued/instant/vector casts, временные и ресурсные условия, отмена действий.
2. Ability/item contract: готовность, дальности/target behavior, cast phase, channel,
   mute/silence/disable, immunity, порядок items, cooldown/потеря цели.
3. Для Invoker/Meepo/Arc Warden — отдельное состояние invoke/клонов/управляемых юнитов.
4. Для Armlet — своевременные attack/projectile/DoT observations и реальная модель окна;
   исторический OnProjectile/OnUnitAnimation из Eroica не создаёт эти события в FDA.
5. Unit/mock-тесты каждого сценария и проверка демо на текущем клиенте.

Не заменять это бесконтрольной последовательностью Q/W/E/R или отключением guards.
Порог HP/список имён модификаторов не равен предсказанию всего смертельного урона.

## Проверенные, но не скопированные репозитории

ivanius51/UmbrellaD2LUA: GPL-3.0, старые Lua для внешнего API; отдельные Invoker/Autohook.
ellysh/dota2-combo-bots: GPL-3.0, старый bot API.
VickTheRock/Ensage-scripts и bundlepak/useful: лицензия в GitHub metadata не указана;
возможность свободного переноса не предполагается. Часть README/алгоритмов устарела.
Рекламные страницы «Free 2026 / anti-ban» не использовались как доказательство исходников.

Файлы установленной Dota при исследовании не читались и не изменялись.
Не запускались скачанные скрипты, не добавлялся обход античита/авторизации.

## ESP17: дополнительные Umbrella источники

Nerve11/Umbrella-Scripts-Lua, MIT, commit fbdf73985821555a94646d1a99216682324f02ae:
проверены и включены как reference Puck Phase Shift, Dawnbreaker Solar Guardian и TA Meld Gate.
README указывает uczoneAPIv2. Скрипты используют Menu/Renderer/Panorama и события внешнего
runtime. В FDA они НЕ подключены и не выполняются. Свежий commit сам по себе не означает
совместимость с текущей Dota или готовность native port.
Svotin/umbrella найден; лицензия не указана, код не скопирован как свободный.

Историческое состояние ESP17: в меню был только Huskar/Armlet. COMBO18 добавляет
9 экспериментальных native basic sequences, не полный порт сторонних модулей.
Полный каталог 127 сохранён отдельно для советника, таблиц и source coverage.
Полный all-hero перенос НЕ реализован. В coverage отдельно указаны базовые
последовательности, full-upstream-port=false и live-verified=false.

## COMBO18 текущий статус

Собственный общий C++ quickcast executor + 9 экспериментальных basic profiles;
не исполнение reference Lua/Rust и не полный перенос их условий/механик.
Включение базового профиля в меню не означает live совместимость или попадание.
См. README-COMBO18.txt, QA-COMBO18.txt и COMBO-SOURCE-COVERAGE.json.
