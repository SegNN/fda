ESP22 — полный перенос исходного GUI из присланного UmbrellaMenu.

Это НЕ заново нарисованный фиолетовый макет ESP21. В src/umbrella включён
полный menu.cpp оригинала (назван umbrella_menu.cpp, чтобы MSVC не перезаписал
obj файла меню базы), оригинальные Lato Regular/Bold, Font Awesome и заголовки.
Оригинальные карточки/окно Settings/иконки/попапы/контекстные меню/темы/анимации
используют исходные функции. Полный НЕИЗМЕНЁННЫЙ standalone-проект с его ImGui
сохранён в references/umbrella-menu-original-full/source для сравнения.
Архивный EXE не запускался и не входит в комплект.

СБОРКА
Распакуйте ВЕСЬ архив в новую папку. Запустите build.bat с MSVC x64/Windows SDK.
Результат при успешной сборке: build/obsdota2.dll. Готовой проверенной DLL в
комплекте нет: здесь Linux, сборка MSVC и работа внутри Dota НЕ проверены.
Файлы src/umbrella/umbrella_menu.cpp и font_data.cpp включены в build.bat.
Сначала проверяются все необходимые файлы. Одинаковых имён obj больше нет.
Не заменяйте только menu.cpp — нужен весь комплект.

УПРАВЛЕНИЕ
INS: открыть/закрыть. Шестерёнка внизу левой рейки: отдельное Settings.
Шестерёнки у строк: оригинальные размер/прозрачность/тема/hover настройки.
ПКМ по строке: исходное контекстное меню/бинды; поддержанные клавиатурные
бинды работают и при закрытом меню с фокусом игры. Menu Bind меняет родной
bind меню; Escape не оставляет меню без возможности открыть (возврат INS).
F7: загрузить выбранный существующий профиль базы, НЕ перезагрузить Lua.
Utility -> Base section Profiles -> Local profiles: Save/Load.
Сохраняются родные настройки и оригинальные настройки GUI/слоёв/gear/биндов.
Подтверждения безопасного quickcast и адреса visual controls по-прежнему
не заменяются автоматическим определением и не превращаются в live-проверку.
Окна перемещаемые, при масштабе/разрешении ограничиваются границами экрана.
На 1352x765 эффективный масштаб подгоняется, чтобы Settings не ушло за экран.

РАБОТАЮЩИЕ ПРИВЯЗКИ В БАЗЕ
Heroes Overlay: Items/Skills/Modifiers Enable, Show On (Enemies/Allies/Local),
Align (Top/Bottom/Left/Right), Minified, размер, обычная/hover прозрачность,
Theme Colors (нейтральные рамки) и OnHover Radius -> настоящий World HeroInfo.
Minified убирает сегменты уровней; не меняет расчёт готовности способности.
Custom Bars Health/Mana используют только прочитанные значения; неизвестная
мана не рисуется как ноль. Невидимый враг не получает придуманных live-данных.
Верхний HUD базы остаётся компактным: HP/мана и квадраты скиллов с КД.
Info Screen -> Info Overlay: TopPanel; Camera: существующие verified-path
visual controls; Notifications: существующие уведомления; Ward Helper: варды.
Abilities / Scripts -> Module: Armlet для своего героя и девять native basic
burst профилей. Это формы существующих модулей, не поддельные Lua-скрипты.
Utility -> Base section / Base module открывает все существующие формы базы:
боевые функции, конфиги, косметику, draft, погоду, диагностику и т.д.
Armlet/базовый прокаст и все защитные проверки ESP21 сохранены; новый GUI не
делает их входящим damage predictor и не гарантирует попадание/выживание.

ЧЕГО В ЭТОМ АРХИВЕ НЕТ
Исходный UmbrellaMenu — standalone GUI replica, НЕ полный исходник Umbrella
чита/его Lua runtime. В базе нет Serverside/Clientside/Developer скриптового
движка и проверенного reader опыта: Enable Scripts / Experience отключены
с пояснением. Остальные неподключённые страницы явно сообщают недоступность.
Copy Lua Path сохраняет исходную функцию копирования UI-пути, не запускает Lua.
Menu Blur Factor — исходный фон/затемнение/свечение, НЕ заявленный GPU blur сцены.
Точные текущие адреса камеры/погоды автоматически НЕ найдены.
Игровой рендер остаётся overlay без depth-occlusion; гарантии отсутствия
перекрытия игрового мира нет. Существующий HUD clipping сохранён.

ПРОВЕРКИ
27 групп Linux portable/mock, отдельно полный исходный GUI с реальными
ImGui mouse IO; glyph Font Awesome; закрытое меню/edge бинды/reload;
round-trip настроек/gear/биндов (включая Theme Colors), отказ при ошибочном
вводе; реальные layer align/size/minified/alpha и mana-read guards.
Полный native menu.cpp также компилировался и рендерился с заменой только
Windows/game-сервисов на mocks; формы Armlet/Lina/Camera/Profile — настоящие.
PNG в tests/ — синтетический C++ ImGui render, НЕ скриншоты live Dota.
Детали и логи в QA-ESP22.txt. Нет Windows/GPU/Steam/live input проверки.
