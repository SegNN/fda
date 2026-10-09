# Lotus: фактический статус FDA ESP17

Полный исходный архив — это комплект текущего FDA, не реализация всех пунктов рекламы.
Скриншоты не являются исходниками или доказательством корректного алгоритма.
Ни один модуль здесь не имеет нового подтверждения полного рабочего матча/Windows build.

| Запрошенный блок | Текущее состояние исходников | Что отсутствует/ограничено |
| --- | --- | --- |
| ESP / HeroInfo | Есть readers, HP-текст, предметы/способности, уровни сегментами, эффекты/статусы/иллюзии и mock geometry tests | Native HP rect не извлекается; все эффекты/классы не гарантированы; нужен live QA |
| Информация под портретами | Есть Top renderer с привязкой topanchor и cooldown icons | Полная устойчивость якорей, отдельная гарантированная TP/ult система не подтверждены |
| Карта / события | Есть уведомления наблюдаемых рун, видимых вардов и Рошана | Полного потока teleport/particle/следов в fog нет; нельзя придумать скрытое серверное состояние |
| Камера / траектории / угрозы | Новой реализации в ARM14 нет | Валидированные camera controls, наблюдение снарядов, timing/trajectory model |
| Иллюзии | Есть чтение/метка иллюзии | Изменение масштаба/моделей из рекламного списка не реализовано этим этапом |
| Фарм | Есть ограниченные last-hit/deny настройки и нижняя оценка damage с armor, bounty HUD | Projectile time, future creep HP, все proc/modifier effects и полноценный orbwalking не моделируются |
| AutoStack / маршруты | Не реализовано | Camp graph, clock/window model, управляемые юниты, route recorder/editor, order adapter и live QA |
| JungleBot | Не реализовано | Маршруты, camp visibility, агро, retreat/pathing и контроль приказов |
| CreepBlock | Не реализовано | Предсказание движения/коллизий и контроль приказов |
| Геройские скрипты | Есть helper catalog и ограниченный manual kill profile; Armlet только свой Huskar | Не набор рабочих 123+ combos; Techies и Arc Warden clone control не реализованы |
| Armlet против урона | ARM13 completion fix + ARM14 diagnostic candidate reader | Нет live field resolver, полной подписки на damage/attack/projectile events и safe OFF window prediction |
| Evader | Не реализовано | События угроз, timing, movement/items/defensive orders, whitelist подтверждённых угроз |
| Net Worth / покупки | Не реализовано полностью | Полный player economy feed; цена прочитанных предметов не равна полному net worth |
| Roshan | Есть наблюдаемое здоровье/таймеры и события | Данные остаются unknown, если нет читаемого подтверждения |
| Dota Plus / косметика | GC-косметика отключена; каталог/локальный UI не означает владение или equip | Не реализовано настоящее получение прав на предметы; стабильный локальный cosmetic renderer со стилями отсутствует |
| RU/EN / config / FPS | Есть RU/EN, UI settings/profile save/load/hotkeys, UI FPS watermark | В watermark ping/MMR остаются --; полноценный перенос/import/export configs из рекламы не заявляется |
| ClickGUI | Существующее меню сохранено | Новый точный визуальный клон Lotus не создавался |

## Обязательная последовательность для damage-aware Armlet

1. Read-only сбор кандидатов: **добавлен в ARM14**. Нет автонажатий по этим данным.
2. Runtime schema offsets + проверка на конкретной сборке/изменении значений: **не завершено**.
3. Источник эффекта, ability level, resistance/barrier/damage flags и семантика тика: **не завершено**.
4. Своевременные attack/projectile/cast observations и uncertainty budget: **не завершено**.
5. Модель безопасного окна OFF/ON и tests для отдельных подтверждённых угроз: **не завершено**.
6. Демо/live acceptance: **не выполнено**.

Не удалять guards и не выдавать HP threshold или список modifier names за пункты 2–6.
Нельзя гарантировать выживание под любым смертельным уроном даже с полной моделью.

## ESP15 дополнение

Удалена heading-полоса. Добавлен display-only 10s fog marker и native-visibility gating геройского ESP. Полный дизайн Octarine НЕ перенесён. Добавлен manual draft-role advisor; auto draft reader/auto pick НЕ реализованы. All-hero combos и Armlet не изменены.

## ESP16 дополнение

Круг заполняется аватаркой сохранённого героя, таймер снизу. Включены MIT reference snapshots для геройских помощников; source coverage для всех 127 героев НЕ означает working combos. Автопрокасты и Armlet не изменены.

## ESP17 актуальное дополнение

Неподключённые геройские пункты убраны из меню, остаётся Huskar/Armlet.
Draft advisor расширен: read-only experimental PlayerResource reader, matched-frame fallback,
manual fallback, published position-weight assignment, lane/partner advice, own already-picked
hero plan. Current live static offsets NOT verified; no hero selection or movement orders.
No all-hero combos integrated. Umbrella sources remain reference only.

## COMBO18: текущий промежуточный этап

Добавлены 9 ограниченных базовых native spell sequences; Huskar Armlet отдельно.
Общий quickcast executor не заменяет полный перенос всех найденных скриптов.
Остальные 118 героев без native combo sequence. Windows/live unverified.
Текущий статус и инструкции: README-COMBO18.txt. Старые разделы — исторические.

## ESP19 current merged release

COMBO18 9 basic profiles and ARM18 cycle corrections are both retained.
Added candidate local-team NPC bit table reader (static offsets/index mapping, NOT live verified).
World/top/automation use one visibility decision; zero model mask no longer proves hidden.
Frozen last-seen world ring; no hidden live-coordinate tracking.
Compact horizontal player-slot cards at top, horizontal rows above world HP bars, top-band exclusion.
Windows/live testing still missing. README-ESP19.txt is current; earlier entries are historical.
