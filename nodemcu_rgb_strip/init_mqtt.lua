last_move = tmr.now()

local function init_settings()
    if file.open("setting.json", "r") then
        local json = sjson.decode(file.read())
        led_r = tonumber(json["led_r"])
        led_g = tonumber(json["led_g"])
        led_b = tonumber(json["led_b"])
        detect_moves = (json["detect_moves"] == "1")
        delay_move = tonumber(json["delay_move"])
        led_is_on = (json["led_is_on"] == "1")
        file.close()
    else
        print("Settings file not found!")
        led_r = 1023
        led_g = 1023
        led_b = 1023
        detect_moves = false
        delay_move = 300000000
        led_is_on = false
    end
end

local function write_settings(name, value)
    local json = {}
    if file.open("setting.json", "r") then
        json = sjson.decode(file.read())
        file.close()
    else
         print("Settings file not found! (write_settings)")
    end
    
    json[name] = value
    local ok, jsonw = pcall(sjson.encode, json)
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

init_settings()

if file.open("private.json", "r") then
    local json = sjson.decode(file.read())        
    m = mqtt.Client("nodemcu_rgb", 120, json["mqtt_login"], json["mqtt_password"])
    file.close()
end

m:lwt("/lwt", "offline", 0, 0)
m:on("connect", function(client) print ("connected") end)
m:on("offline", function(client) print ("offline") end)
m:on("message", function(client, topic, data) 
  if data ~= nil then
    if topic == "light/all" then
        if data == "0" then
          ledoff()
          collectgarbage()
        end
    elseif topic == "rgb/kitchen" then
        local pref = string.sub(data,1,1)
        if pref == "C" then
            if data == "C000000000000" then
                ledoff()
            else
                led_r = tonumber(string.sub(data,2,5))
                led_g = tonumber(string.sub(data,6,9))
                led_b = tonumber(string.sub(data,10,13))
                ledon()
            end
        elseif pref == "D" then
            delay_move = tonumber(string.sub(data,2))
            write_settings("delay_move", delay_move)
        elseif pref == "M" then
            local strsub = string.sub(data,2,2)
            detect_moves = (strsub == "1")
            write_settings("detect_moves", strsub)
        end
    end
  end
end)

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


pwm.setup(5, 1000, led_r)
pwm.setup(6, 1000, led_g)
pwm.setup(7, 1000, led_b)
pwm.start(5)
pwm.start(6)
pwm.start(7)

function ledon()
    pwm.setduty(5, led_r) 
    pwm.setduty(6, led_g)
    pwm.setduty(7, led_b)
    led_is_on = true
    last_move = tmr.now()
    
    write_settings("led_r", led_r)
    write_settings("led_g", led_g)
    write_settings("led_b", led_b)
    write_settings("led_is_on", "1")
end

function ledoff()
    pwm.setduty(5, 0) 
    pwm.setduty(6, 0)
    pwm.setduty(7, 0)
    led_is_on = false
    write_settings("led_is_on", "0")
end

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