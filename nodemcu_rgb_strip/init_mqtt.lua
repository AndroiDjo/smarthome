last_move = tmr.now()
led_is_on = false

function ledon()
    pwm.setduty(5, led_r) 
    pwm.setduty(6, led_g)
    pwm.setduty(7, led_b)
    led_is_on = true
    last_move = tmr.now()
end

function ledoff()
    pwm.setduty(5, 0) 
    pwm.setduty(6, 0)
    pwm.setduty(7, 0)
    led_is_on = false
end

function write_settings(json)
    local jsonf = {}
    if file.open("setting.json", "r") then
        jsonf = sjson.decode(file.read())
        file.close()
    else
         print("Settings file not found! (write_settings)")
    end
    
    for k,v in pairs(json) do jsonf[k] = v end
    
    local ok, jsonw = pcall(sjson.encode, jsonf)
    if ok then
      if file.open("setting.json", "w") then
        file.write(jsonw)
        file.close()
      else
        print("failed to write settings!")
      end
    else
      print("failed to encode!")
    end
end

function processActions(jm)
    if jm.color ~= nil then
        led_r = jm.color.r
        led_g = jm.color.g
        led_b = jm.color.b
    end
    
    if jm.power ~= nil then
        if jm.power == 1 then
            ledon()
        else
            ledoff()
        end
    end
    
    if jm.detectmove ~=nil then
        detect_moves = (jm.detectmove == 1)
    end
    
    if jm.delaymove ~=nil then
        delay_move = jm.delaymove
    end
end

function onMsg(client, topic, data) 
  if data ~= nil then
    local jm = sjson.decode(data)
    processActions(jm)
    write_settings(jm)
  end
end

local function init_settings()
    if file.open("setting.json", "r") then
        local json = sjson.decode(file.read())
        file.close()
        processActions(json)
    else
        print("Settings file not found!")
        led_r = 1023
        led_g = 1023
        led_b = 1023
        detect_moves = true
        delay_move = 300000000
    end
end

if file.open("private.json", "r") then
    local json = sjson.decode(file.read())        
    m = mqtt.Client("nodemcu_rgb", 120, json["mqtt_login"], json["mqtt_password"])
    file.close()
end

m:lwt("/lwt", "rgb/kitchen offline", 0, 0)
m:on("connect", function(client) print ("connected") end)
m:on("offline", function(client) print ("offline") end)
m:on("message", onMsg)

if file.open("private.json", "r") then
    local json = sjson.decode(file.read())        
    m:connect(json["mqtt_host"], json["mqtt_port"], 0, function(client)
      print("connected")
      client:subscribe("rgb/kitchen", 0)
      client:subscribe("light/all", 0)
    end,
    function(client, reason)
      print("failed reason: " .. reason)
    end)
    file.close()
end

init_settings()
pwm.setup(5, 1000, led_r)
pwm.setup(6, 1000, led_g)
pwm.setup(7, 1000, led_b)
pwm.start(5)
pwm.start(6)
pwm.start(7)

mytimer = tmr.create()
mytimer:register(1000, tmr.ALARM_AUTO, function (t)
                                           if detect_moves then
                                               if tmr.now() - last_move > delay_move then
                                                  ledoff()
                                               end
                                           end
                                       end)
mytimer:start()

gpio.mode(2, gpio.INPUT)
gpio.trig(2, "high", function(level, when)
                        if(detect_moves) then
                            last_move = tmr.now()
                            if not led_is_on then
                                ledon()
                            end
                        end
                     end)