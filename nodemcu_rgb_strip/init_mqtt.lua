firstinit = true
last_move = tmr.now()
led_is_on = false
led_r = 1023
led_g = 1023
led_b = 1023
detect_moves = true
delay_move = 300000000
led_off_callback = false

pon_settings = {}
pon_settings["gradtime"] = 500
pon_settings["gradsteps"] = 50
pon_settings["gradient"] = 1

poff_settings = {}
poff_settings["gradtime"] = 1000
poff_settings["gradsteps"] = 100
poff_settings["gradient"] = 1
poff_color = {}
poff_color["r"] = 0
poff_color["g"] = 0
poff_color["b"] = 0
poff_settings["tocolor"] = poff_color

grad_on = false
grad_loop = true
grad_time = 1000
grad_steps = 100
grad_step_time = 10
curstep = 1
led_from_r = led_r
led_from_g = led_g
led_from_b = led_b
led_to_r = 0
led_to_g = 0
led_to_b = 0
delta_r = -1
delta_g = -1
delta_b = -1

local function checkLimits(led)
    local num = led
    if num > 1023 then
        num = 1023
    elseif num < 1 then
        num = 0
    end
    return num
end

gradtmr = tmr.create()
gradtmr:register(grad_step_time, tmr.ALARM_AUTO, function (t)
                                           if curstep > grad_steps or not grad_on or not led_is_on then
                                               gradtmr:stop()
                                               if led_off_callback then
                                                  led_off_callback = false
                                                  if led_r == 0 or led_g == 0 or led_b == 0 then
                                                    led_is_on = false
                                                  end
                                                  led_to_r = poff_settings.btocolorr
                                                  led_to_g = poff_settings.btocolorg
                                                  led_to_b = poff_settings.btocolorb
                                                  grad_time = poff_settings.bgradtime
                                                  grad_steps = poff_settings.bgradsteps
                                                  grad_loop = (poff_settings.bgradloop == 1)
                                                  grad_on = (poff_settings.bgradient == 1)                                                  
                                               elseif grad_loop and grad_on and led_is_on then
                                                  reverseGradient()
                                               end
                                           else
                                               if led_from_r == led_to_r and led_from_g == led_to_g and led_from_b == led_to_b then
                                                   curstep = grad_steps
                                                   grad_loop = false
                                                   led_r = led_to_r
                                                   led_g = led_to_g
                                                   led_b = led_to_b
                                               elseif curstep >= grad_steps then
                                                   led_r = led_to_r
                                                   led_g = led_to_g
                                                   led_b = led_to_b
                                               else
                                                   led_r = led_r + delta_r
                                                   led_g = led_g + delta_g
                                                   led_b = led_b + delta_b
                                               end
                                               led_r = checkLimits(led_r)
                                               led_g = checkLimits(led_g)
                                               led_b = checkLimits(led_b)
                                               pwm.setduty(5, led_r)
                                               pwm.setduty(6, led_g)
                                               pwm.setduty(7, led_b)
                                               curstep = curstep + 1
                                           end
                                       end)
                                       
function ledon()
    led_from_r = led_r
    led_from_g = led_g
    led_from_b = led_b
    if led_to_r == 0 and led_to_g == 0 and led_to_b == 0
        and led_from_r == 0 and led_from_g == 0 and led_from_b == 0 then
            led_to_r = 1023
            led_to_g = 1023
            led_to_b = 1023
    end
    led_is_on = true
    collectgarbage()
    if grad_on then
        doGradient()
    else
        processActions(pon_settings)
    end
    last_move = tmr.now()
end

function ledoff()
    if led_is_on then
        led_from_r = led_r
        led_from_g = led_g
        led_from_b = led_b
        poff_settings["btocolorr"] = led_to_r
        poff_settings["btocolorg"] = led_to_g
        poff_settings["btocolorb"] = led_to_b
        poff_settings["bgradtime"] = grad_time
        poff_settings["bgradsteps"] = grad_steps
        poff_settings["bgradloop"] = (grad_loop and 1 or 0)
        poff_settings["bgradient"] = (grad_on and 1 or 0)
        gradtmr:stop()
        collectgarbage()
        led_off_callback = true
        processActions(poff_settings)
    end
end

function calcGradient()
    if grad_time < 10 then
        grad_time = 10
    end
    
    if grad_steps > grad_time then
        grad_steps = grad_time
    end

    grad_step_time = grad_time / grad_steps
    if grad_step_time < 10 then
        grad_step_time = 10
        grad_steps = grad_time / grad_step_time
    end
    delta_r = (led_to_r - led_from_r) / grad_steps
    delta_g = (led_to_g - led_from_g) / grad_steps
    delta_b = (led_to_b - led_from_b) / grad_steps
    led_r = led_from_r
    led_g = led_from_g
    led_b = led_from_b
end

function doGradient()
    gradtmr:stop()
    collectgarbage()
    curstep = 1
    calcGradient()
    gradtmr:interval(grad_step_time)
    gradtmr:start()
end

function reverseColors()
    local r = led_to_r
    local g = led_to_g
    local b = led_to_b
    led_to_r = led_from_r
    led_to_g = led_from_g
    led_to_b = led_from_b
    led_from_r = r
    led_from_g = g
    led_from_b = b
end

function reverseGradient()
    reverseColors()
    collectgarbage()
    doGradient()
end

function write_settings(json)
    local jsonf = {}
    if file.open("setting.json", "r") then
        jsonf = sjson.decode(file.read())
        collectgarbage()
        file.close()
    end
    
    for k,v in pairs(json) do jsonf[k] = v end
    
    local ok, jsonw = pcall(sjson.encode, jsonf)
    if ok then
      collectgarbage()
      if file.open("setting.json", "w") then
        file.write(jsonw)
        file.close()
      end
    end
end

function processActions(jm)
    if jm.tocolor ~= nil then
        led_to_r = jm.tocolor.r
        led_to_g = jm.tocolor.g
        led_to_b = jm.tocolor.b
    end
    
    if jm.fromcolor ~= nil then
        led_from_r = jm.fromcolor.r
        led_from_g = jm.fromcolor.g
        led_from_b = jm.fromcolor.b
    end
    
    if jm.gradtime ~= nil then
        grad_time = jm.gradtime
    end
    
    if jm.gradsteps ~= nil then
        grad_steps = jm.gradsteps
    end
    
    if jm.gradloop ~=nil then
        grad_loop = (jm.gradloop == 1)
    end
    
    if jm.gradient ~= nil then
        if jm.gradient == 1 then
            grad_on = true
            doGradient()
        else
            grad_on = false
            grad_loop = false
        end
    end
    
    if jm.detectmove ~=nil then
        detect_moves = (jm.detectmove == 1)
    end
    
    if jm.delaymove ~=nil then
        delay_move = jm.delaymove
    end
    
    if jm.ponsettings ~= nil then
        pon_settings = jm.ponsettings
    end
    
    if jm.poffsettings ~= nil then
        poff_settings = jm.poffsettings
    end
    
    if jm.cpufreq ~= nil then
        if jm.cpufreq == 160 then
            node.setcpufreq(node.CPU160MHZ)
        elseif jm.cpufreq == 80 then
            node.setcpufreq(node.CPU80MHZ)
        end
        collectgarbage()
    end
    
    if jm.power ~= nil then
        if jm.power == 1 then
            ledon()
        else
            ledoff()
        end
    end
end

function onMsg(client, topic, data) 
gradtmr:stop()
  if data ~= nil then
    local jm = sjson.decode(data)
    collectgarbage()
    processActions(jm)
    write_settings(jm)
  end
end

function get_settings()
    local json = {}
    if file.open("setting.json", "r") then
        json = sjson.decode(file.read())
        collectgarbage()
        file.close()
    end
    return json
end

function init_settings()
    if firstinit then
        firstinit = false
        local json = get_settings()
        processActions(json)
        
        mytimer = tmr.create()
        mytimer:register(1000, tmr.ALARM_AUTO, function (t)
                                                   if detect_moves and led_is_on then
                                                       if tmr.now() - last_move > delay_move then
                                                          gradtmr:stop()
                                                          ledoff()
                                                       end
                                                   end
                                               end)
        mytimer:start()
    end
end

if file.open("private.json", "r") then
    local json = sjson.decode(file.read())
    collectgarbage()
    m = mqtt.Client("nodemcu_rgb", 120, json["mqtt_login"], json["mqtt_password"])
    file.close()
end


reconntimer = tmr.create()
reconntimer:register(10000, tmr.ALARM_AUTO,
function (t)
    if file.open("private.json", "r") then
        local json = sjson.decode(file.read())        
        collectgarbage()
        m:connect(json["mqtt_host"], json["mqtt_port"], 0, function(client)
          reconntimer:stop()
          client:subscribe("rgb/kitchen", 0)
          client:subscribe("light/all", 0)
          init_settings()
        end,
        function(client, reason)
          print("failed reason: " .. reason)
        end)
        file.close()
    end
end)

m:lwt("/lwt", "rgb/kitchen offline", 0, 0)
m:on("connect", function(client) print ("connected") end)
m:on("offline", function(client) print ("offline") end)
m:on("message", onMsg)

reconntimer:start()

pwm.setup(5, 1000, 0)
pwm.setup(6, 1000, 0)
pwm.setup(7, 1000, 0)
pwm.start(5)
pwm.start(6)
pwm.start(7)

gpio.mode(2, gpio.INPUT)
gpio.trig(2, "high", function(level, when)
                        if(detect_moves) then
                            last_move = tmr.now()
                            if not led_is_on then
                                local json = get_settings()
                                processActions(json)
                            end
                        end
                     end)