-- main.lua for LAHEE UI
local http = require("socket.http")

-- Simple JSON decoder (minimal implementation for LAHEE)
-- In a real scenario, we'd bundle a full json.lua, but this handles the basic LAHEE response
local json = {}
function json.decode(s)
    local games = {}
    for id, title in s:gmatch('"ID":(%d+).-"Title":"(.-)"') do
        table.insert(games, {ID = id, Title = title})
    end
    return {games = games}
end

local WIDTH = 640
local HEIGHT = 480
local API_URL = "http://127.0.0.1:8000/dorequest.php"

local state = "LOADING"
local games = {}
local selected_idx = 1
local error_msg = ""

function fetch_info()
    local body, code = http.request(API_URL, "r=laheeinfo")
    if code == 200 then
        local data = json.decode(body)
        if data and data.games then
            games = data.games
            state = "LIST"
        else
            state = "ERROR"
            error_msg = "Invalid data from server"
        end
    else
        state = "ERROR"
        error_msg = "Could not connect to LAHEE Server"
    end
end

function love.load()
    love.window.setMode(WIDTH, HEIGHT)
    font = love.graphics.newFont(18)
    large_font = love.graphics.newFont(24)
    fetch_info()
end

function love.keypressed(key)
    if key == "escape" or key == "b" or key == "back" then
        love.event.quit()
    elseif key == "up" then
        selected_idx = math.max(1, selected_idx - 1)
    elseif key == "down" then
        selected_idx = math.min(#games, selected_idx + 1)
    end
end

function love.draw()
    love.graphics.clear(30/255, 30/255, 30/255)
    love.graphics.setFont(large_font)
    love.graphics.setColor(1, 1, 1)
    
    if state == "LOADING" then
        love.graphics.print("Loading LAHEE Data...", 20, 20)
    elseif state == "ERROR" then
        love.graphics.setColor(1, 0.4, 0.4)
        love.graphics.print("ERROR", 20, 20)
        love.graphics.setColor(1, 1, 1)
        love.graphics.setFont(font)
        love.graphics.print(error_msg, 20, 60)
        love.graphics.print("Press B to exit", 20, 100)
    elseif state == "LIST" then
        love.graphics.print("LAHEE Games", 20, 20)
        love.graphics.setFont(font)
        
        if #games == 0 then
            love.graphics.print("No games found.", 20, 80)
        else
            for i, game in ipairs(games) do
                if i == selected_idx then
                    love.graphics.setColor(1, 1, 0.4)
                else
                    love.graphics.setColor(0.8, 0.8, 0.8)
                end
                love.graphics.print(game.Title or "Unknown", 20, 80 + (i-1) * 30)
            end
        end
        love.graphics.setColor(0.6, 0.6, 0.6)
        love.graphics.print("Press B to exit", 20, 440)
    end
end
