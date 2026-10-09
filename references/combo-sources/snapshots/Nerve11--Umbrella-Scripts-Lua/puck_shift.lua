local PuckScript = {}

-- Переменные
local localHero = nil
local gameTime = 0
local projectileCounter = 0

-- Создание меню
local mainMenu = Menu.Create("Heroes", "Hero List", "Puck")
local phaseShiftMenu = mainMenu:Create("Auto Phase Shift"):Create("Global")
local settings = {}

-- Настройки скрипта
settings.masterEnable = phaseShiftMenu:Switch("Enable Script", true, "🗸")
local mainSettings = mainMenu:Create("Auto Phase Shift"):Create("Main")
settings.logicEnabled = mainSettings:Switch("Enable Logic", true, "✓")
settings.minProjectileCount = mainSettings:Slider("Min Projectiles to Dodge", 1, 5, 1, 
    function(value) 
        return tostring(value) 
    end)

-- Настройки UI индикатора
db.puckIndicator = db.puckIndicator or {}
local indicatorData = db.puckIndicator
local font = Renderer.LoadFont("Arial", 20, Enum.FontCreate.FONTFLAG_ANTIALIAS)

-- Переменные для анимации индикатора
local indicatorYOffset = 0
local indicatorTargetYOffset = 0
local animationSpeed = 1
local fadeInSpeed = 5
local fadeOutSpeed = 15
local activeColor = Color(255, 0, 0, 0)
local hoverColor = Color(175, 175, 175, 0)

-- UI элементы
local indicatorXPos = nil
local indicatorWidth = nil
local smoothingFactor = 0.1
local uiInitialized = false
local centerPanel = nil
local abilityBevel = nil
local abilityButton = nil

-- Трекинг снарядов
local activeProjectiles = {}
local lastCastTime = 0

-- Функция инициализации UI
local function initializeUI()
    if uiInitialized then 
        return 
    end
    
    centerPanel = Panorama.GetPanelByName("center_bg")
    if not centerPanel then 
        return 
    end
    
    local ability2Panel = Panorama.GetPanelByName("Ability2")
    if ability2Panel then
        abilityBevel = ability2Panel:FindChildTraverse("AbilityBevel")
        abilityButton = ability2Panel:FindChildTraverse("AbilityButton")
    end
    
    uiInitialized = true
end

-- Функция плавной анимации значений
local function smoothTransition(current, target, speed)
    if current < target then
        return math.min(current + speed, target)
    elseif current > target then
        return math.max(current - speed, target)
    end
    return current
end

-- Получение позиции и размеров UI элемента
local function getElementPosition(element)
    local xOffset, yOffset = 0, 0
    local currentElement = element
    
    while currentElement do
        xOffset = xOffset + currentElement:GetXOffset()
        yOffset = yOffset + currentElement:GetYOffset()
        currentElement = currentElement:GetParent()
    end
    
    return xOffset, yOffset
end

-- Получение полных координат и размеров элемента
local function getElementBounds(element)
    local x, y = getElementPosition(element)
    local bounds = element:GetBounds()
    local width = tonumber(bounds.w) or 0
    local height = tonumber(bounds.h) or 0
    return x, y, width, height
end

-- Получение локального героя
local function getLocalHero()
    if not localHero then
        localHero = Heroes.GetLocal()
    end
    return localHero
end

-- Подсчет активных снарядов
function GetActiveProjectileCount()
    local count = 0
    local currentTime = GameRules.GetGameTime()
    
    for projectileId, projectileData in pairs(activeProjectiles) do
        if (currentTime - projectileData.time) < 0.5 then
            count = count + 1
        else
            -- Удаляем устаревшие снаряды
            activeProjectiles[projectileId] = nil
        end
    end
    
    return count
end

-- Обработчик новых снарядов
PuckScript.OnProjectile = function(projectile)
    local hero = getLocalHero()
    if not hero then 
        return 
    end
    
    -- Проверяем, что это Puck
    if NPC.GetUnitName(hero) ~= "npc_dota_hero_puck" then 
        return 
    end
    
    -- Проверяем, что скрипт включен
    if not settings.masterEnable:Get() or not settings.logicEnabled:Get() then 
        return 
    end
    
    -- Проверяем, что герой жив
    if not Entity.IsAlive(hero) then 
        return 
    end
    
    local source = projectile.source
    local target = projectile.target
    
    -- Если снаряд летит в нашего героя от врага
    if target == hero and source and not Entity.IsSameTeam(hero, source) then
        local projectileId = projectile.id
        
        -- Генерируем ID если его нет
        if not projectileId or projectileId == 0 then
            projectileCounter = projectileCounter + 1
            projectileId = "generated_" .. projectileCounter
        end
        
        -- Сохраняем информацию о снаряде
        activeProjectiles[projectileId] = {
            source = source,
            time = GameRules.GetGameTime()
        }
    end
end

-- Основной цикл обновления
PuckScript.OnUpdate = function()
    -- Проверяем, что скрипт включен
    if not settings.masterEnable:Get() or not settings.logicEnabled:Get() then 
        return 
    end
    
    local hero = getLocalHero()
    if not hero then 
        return 
    end
    
    -- Проверяем, что это Puck
    if NPC.GetUnitName(hero) ~= "npc_dota_hero_puck" then 
        return 
    end
    
    -- Проверяем, что герой жив
    if not Entity.IsAlive(hero) then 
        return 
    end
    
    local currentTime = GameRules.GetGameTime()
    
    -- Очищаем старые снаряды
    for projectileId, projectileData in pairs(activeProjectiles) do
        if (currentTime - projectileData.time) > 1 then
            activeProjectiles[projectileId] = nil
        end
    end
    
    -- Проверяем, нужно ли использовать Phase Shift
    if GetActiveProjectileCount() >= settings.minProjectileCount:Get() then
        -- Проверяем кулдаун (не чаще раза в секунду)
        if (currentTime - lastCastTime) > 1 then
            local phaseShiftAbility = NPC.GetAbility(hero, "puck_phase_shift")
            
            if phaseShiftAbility and Ability.IsReady(phaseShiftAbility) then
                -- Используем Phase Shift
                Ability.CastNoTarget(phaseShiftAbility)
                lastCastTime = currentTime
                
                -- Очищаем список снарядов после использования
                activeProjectiles = {}
            end
        end
    end
end

-- Отрисовка индикатора
PuckScript.OnDraw = function()
    local hero = getLocalHero()
    if not hero or NPC.GetUnitName(hero) ~= "npc_dota_hero_puck" then 
        return 
    end
    
    -- Проверяем, что скрипт включен
    if not settings.masterEnable:Get() then 
        return 
    end
    
    -- Проверяем, что Puck выбран
    local selectedUnits = Player.GetSelectedUnits(Players.GetLocal())
    if not selectedUnits then 
        return 
    end
    
    local isPuckSelected = false
    for _, unit in ipairs(selectedUnits) do
        if unit == hero then
            isPuckSelected = true
            break
        end
    end
    
    if not isPuckSelected then 
        return 
    end
    
    -- Инициализируем UI если нужно
    if not uiInitialized then
        initializeUI()
    end
    
    -- Проверяем, что все UI элементы доступны
    if not (uiInitialized and centerPanel and abilityBevel and abilityButton) then 
        return 
    end
    
    local isLogicEnabled = settings.logicEnabled:Get()
    
    -- Получаем координаты элементов
    local centerX, centerY, centerWidth, centerHeight = getElementBounds(centerPanel)
    local bevelX, bevelY, bevelWidth, bevelHeight = getElementBounds(abilityBevel)
    local buttonX, buttonY, buttonWidth, buttonHeight = getElementBounds(abilityButton)
    
    -- Инициализируем позицию индикатора
    if indicatorXPos == nil then
        indicatorXPos = bevelX
    end
    if indicatorWidth == nil then
        indicatorWidth = bevelWidth
    end
    
    -- Вычисляем позицию и размеры индикатора
    local indicatorHeight = 5
    local topY = centerY
    local bottomY = topY - 3
    local leftX = indicatorXPos
    local rightX = bottomY + indicatorYOffset
    local width = indicatorWidth
    local rectHeight = topY - rightX
    
    -- Проверяем наведение курсора
    local isCursorOver = Input.IsCursorInRect(leftX, rightX, width, rectHeight)
    
    -- Настройки цвета и анимации
    local baseAlpha = 135
    local epsilon = 0.1
    local isAligned = (math.abs(indicatorXPos - buttonX) < epsilon) and 
                     (math.abs(indicatorWidth - buttonWidth) < epsilon)
    local hoverAlpha = (isCursorOver and isAligned and 255) or 0
    
    -- Анимация цвета
    activeColor.a = smoothTransition(activeColor.a, baseAlpha, fadeOutSpeed)
    hoverColor.a = smoothTransition(hoverColor.a, hoverAlpha, fadeOutSpeed)
    
    if activeColor.a == 0 then 
        return 
    end
    
    -- Установка цвета в зависимости от состояния
    local targetColor = (isLogicEnabled and {r = 0, g = 255, b = 0}) or {r = 255, g = 0, b = 0}
    activeColor.r = smoothTransition(activeColor.r, targetColor.r, fadeOutSpeed)
    activeColor.g = smoothTransition(activeColor.g, targetColor.g, fadeOutSpeed)
    activeColor.b = smoothTransition(activeColor.b, targetColor.b, fadeOutSpeed)
    
    -- Позиционирование индикатора
    local indicatorRectX = indicatorXPos
    local indicatorRectY = (bottomY - indicatorHeight) + indicatorYOffset
    local indicatorRectWidth = indicatorWidth
    local indicatorRectHeight = indicatorHeight
    
    -- Проверка наведения на индикатор
    local isIndicatorHovered = Input.IsCursorInRect(leftX, rightX, width, rectHeight) or 
                              Input.IsCursorInRect(indicatorRectX, indicatorRectY, indicatorRectWidth, indicatorRectHeight)
    
    -- Анимация сдвига
    local isAtZero = math.abs(indicatorYOffset - 0) < epsilon
    if isIndicatorHovered and isAligned then
        indicatorTargetYOffset = -20
    else
        indicatorTargetYOffset = 0
    end
    
    -- Плавное движение к цели
    local targetX, targetWidth
    if isIndicatorHovered then
        targetX = buttonX
        targetWidth = buttonWidth
    elseif not isIndicatorHovered and not isAtZero then
        targetX = buttonX
        targetWidth = buttonWidth
    else
        targetX = bevelX
        targetWidth = bevelWidth
    end
    
    -- Применение сглаживания
    indicatorXPos = indicatorXPos + ((targetX - indicatorXPos) * smoothingFactor)
    indicatorWidth = indicatorWidth + ((targetWidth - indicatorWidth) * smoothingFactor)
    
    -- Анимация вертикального смещения
    if indicatorYOffset > indicatorTargetYOffset then
        indicatorYOffset = math.max(indicatorYOffset - animationSpeed, indicatorTargetYOffset)
    elseif indicatorYOffset < indicatorTargetYOffset then
        indicatorYOffset = math.min(indicatorYOffset + animationSpeed, indicatorTargetYOffset)
    end
    
    -- Отрисовка тени
    local shadowColor = Color(0, 0, 0, math.min(125, math.floor(activeColor.a)))
    local shadowStart = Vec2(indicatorXPos, bottomY + indicatorYOffset)
    local shadowEnd = Vec2(indicatorXPos + indicatorWidth, topY)
    Render.FilledRect(shadowStart, shadowEnd, shadowColor, 0, Enum.DrawFlags.None)
    
    -- Отрисовка основного индикатора
    local mainRectStart = Vec2(indicatorRectX, indicatorRectY)
    local mainRectEnd = Vec2(indicatorRectX + indicatorRectWidth, indicatorRectY + indicatorRectHeight)
    Render.FilledRect(mainRectStart, mainRectEnd, activeColor, 3, Enum.DrawFlags.RoundCornersTop)
    
    -- Отрисовка тени индикатора
    local shadowStart2 = Vec2(indicatorRectX + 1, indicatorRectY + 1)
    local shadowEnd2 = Vec2((indicatorRectX + indicatorRectWidth) - 3, indicatorRectY + indicatorRectHeight)
    Render.Shadow(shadowStart2, shadowEnd2, activeColor, 20)
    
    -- Отрисовка текста состояния
    if hoverColor.a > 0 then
        local textY1 = bottomY + indicatorYOffset
        local textY2 = topY
        local textCenterY = (textY1 + textY2) * 0.5
        local statusText = (isLogicEnabled and "ON") or "OFF"
        local textSize = Render.TextSize(1, 20, statusText)
        local textX = (indicatorXPos + (indicatorWidth * 0.5)) - (textSize.x * 0.5)
        local textY = textCenterY - (textSize.y * 0.5)
        
        Render.Text(1, 20, statusText, Vec2(textX, textY), hoverColor)
    end
    
    -- Обработка клика для переключения состояния
    if Input.IsCursorInRect(leftX, rightX, width, rectHeight) and 
       Input.IsKeyDownOnce(Enum.ButtonCode.KEY_MOUSE1) then
        settings.logicEnabled:Set(not isLogicEnabled)
    end
end

-- Обработчик загрузки скрипта
PuckScript.OnScriptLoad = function()
    local hero = getLocalHero()
    -- Дополнительная инициализация при необходимости
end

return PuckScript
