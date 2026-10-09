FDA ARM18 — исправление цикла Armlet + подробная история ввода

БАЗА: ESP17, целевой ремонт Armlet. Не расширение all-hero combo suite.
Полная Windows DLL НЕ собрана/не проверена; живая Dota не проверена.
Это не обещание «работает всегда». Полный incoming-hit/projectile predictor НЕ реализован.

КОНКРЕТНЫЕ ИЗМЕНЕНИЯ ЦИКЛА:
1. Убран дополнительный искусственный минимум 100ms между запросом OFF и ON.
   ON отправляется на первом кадре, где OFF реально прочитан и предмет готов.
   Если OFF не подтверждён или предмет не готов — второго нажатия НЕТ.
2. Старый режим требовал HP через секунду выше HP до цикла. Под новым уроном это могло
   ошибочно защёлкнуть Fault даже после успешного ON. Теперь ON подтверждается одновременно
   RAW toggle flag и активным modifier_item_armlet_unholy_strength из verified buff list.
   Затем 1s settling перед rearm; нет требования положительного NET HP под уроном.
   Само наличие ON/модификатора НЕ доказывает величину полученного HP или спасение от удара.
3. Сохранены threshold trigger, 2s minimum cycle spacing, readiness/cooldown, identity,
   selection, focus/menu/pause/owner/state/DoT guards. Нет blind double-tap, late replay,
   принудительного обхода stun/mute, новых переключений OFF под неизвестным DoT timing.
   ON completion после уже подтверждённого OFF не отменяется одним появлением DoT.
4. ON flag без активного эффекта не считается успешным восстановлением: timeout/Fault.

ДИАГНОСТИКА:
- closedTick: HP, threshold, hpLEthreshold, phase, on, ready, effectOn и полный item handle
  из последнего кадра С ЗАКРЫТЫМ МЕНЮ; не смешивается с текущим открытым меню.
- closedGate: чтение unit state, blockedMask, rawRead, cd, active, itemPhase, muted,
  indefinite/frozen и точная причина блокировки.
- armletInput: inputCalls, requestedOff/On, deliveredPairs, failures, lastSent/lastAction.
  SendInput accepted key events != игра действительно переключила предмет.
- Последние 16 событий: пересечение порога, статус/phase changes, действия и ошибки.
- Старый faultContext больше не выглядит текущей ошибкой при firstFault=none.
  История явно маркируется historicalOnly_faultContext. Новая ошибка имеет новый контекст.
- Буфер copied report расширен, чтобы помещалась история.

ЧТО ПОДТВЕРЖДАЮТ ТВОИ СНИМКИ ESP17:
- selected singleton own Huskar, localFrame=1, inventoryRead/buffsRead=1 — прошли.
- 736 > 550 и 485 > 451.471 — на этих снимках пороговый триггер не выполнен.
- State 0x200000106 пересекает старую блокирующую маску: 0x200000100. Смысл всех битов
  в текущем клиенте из этих снимков не доказан; guard не отключался без проверки.
- HP404 ниже451, но menuOpen=1: этот кадр не разрешает ввод.
- Снимки не доказывают, происходили ли нажатия между ними. Поэтому добавлены counters/history.
- Старые death faultContext показывают прошлое HP0 при pending phase; это НЕ доказательство,
  что задержка или HP recovery gate были единственной причиной твоей смерти/несрабатывания.

ПЕРВАЯ ПРОВЕРКА (ТОЛЬКО ДЕМО):
1. Весь архив в НОВУЮ ПАПКУ ПРОЕКТА -> build.bat (MSVC x64, C++ и Windows SDK).
   Не в установленную папку Dota. Старые README/QA — исторические. Готовой DLL нет.
2. В игре убедиться в маркере ARM18. Huskar, Armlet в указанном слоте1–6, реальная native
   клавиша (у тебя Z=90). Сначала вручную проверить, что именно Z переключает этот предмет.
   Подтвердить клавишу в меню; включить helper, выбрать только своего героя.
3. Закрыть чат/консоль и FDA меню, вернуть фокус Dota. Чат/консоль FDA не распознаёт.
   В спокойном демо проверить OFF -> ON. Далее, при низком HP, ON -> OFF -> ON.
4. При неуспехе открыть меню сразу после эпизода и скопировать новую диагностику Armlet.
   Не сбрасывать до копирования. Нужны closedTick, closedGate, armletInput, event[], cycle.
   inputCalls=0 означает, что запрос не дошёл до SendInput; lastSent=2 означает только
   принятую Windows пару событий. on/effectOn/phase показывают подтверждение в игре.
5. При Fault проверить состояние предмета вручную и сбросить цикл; позднее автонажатие
   после потери focus/selection/menu не выполняется. Проверки не обходить.

ИССЛЕДОВАННЫЕ ДРУГИЕ КОДЫ:
- EroicaCpp Lua 3.1: OnProjectile/OnUnitAnimation, оценка атаки, очередь OFF/ON. Использует
  внешние Heroes/NPC/Ability APIs; этих событий/приказов в FDA сейчас нет.
- pless42 TypeScript: API прямо помечены «Предполагаем GameTime»/«Адаптировать под API»;
  глобальный GameTime modulo для tick timing не является проверкой индивидуального DoT.
  Не принят как готовый компилируемый SDK/native adapter.
- pless42 Lua: внешний runtime; hardcoded tick periods; duplicate top-level return at end.
  Не исполнялся, не копировался как рабочий модуль.
- zfael Rust: другой GSI/input runtime, cooldown/critical retry и отдельная Roshan learning
  logic; не универсальный predictor всех атак/DoT. В reference snapshot уже был в ESP17.
Ссылки/ограничения: ARM18-SOURCE-REVIEW.json. Рекламные anti-ban/download страницы не
использованы как доказательство работающего кода. Ничего с них не запускалось.

ОГРАНИЧЕНИЯ:
Этот ремонт НЕ превращает пороговый helper в event-driven Armlet abuse для всех атак,
снарядов и DoT. Для этого FDA нужен native observer таких событий и валидация таймингов
текущей Dota; сейчас их нет. Предсказывать следующую атаку по одному HP снимку нельзя.
Offsets/read semantics не подтверждены на живом клиенте. Возможны отказ ввода/смерть.
Файлы установленной игры не читались/не менялись. GitHub не обновлялся.
