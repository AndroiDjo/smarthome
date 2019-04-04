last_move = tmr.now()
led_is_on = false
led_r = 1023
led_g = 1023
led_b = 1023
detect_moves = true
delay_move = 300000000

grad_on = false
grad_loop = false
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
gradtmr = tmr.create()
gradtmr:register(grad_step_time, tmr.ALARM_AUTO, function (t)
                                           if curstep > grad_steps or not grad_on then
                                               gradtmr:stop()
                                               if grad_loop then
                                                  reverseGradient()
                                               end
                                           else
                                               if curstep == grad_steps then
                                                   led_r = led_to_r
                                                   led_g = led_to_g
                                                   led_b = led_to_b
                                               else
                                                   led_r = led_r + delta_r
                                                   led_g = led_g + delta_g
                                                   led_b = led_b + delta_b
                                               end
                                               print(led_r)
                                               print(node.heap())
                                               pwm.setduty(5, led_r)
                                               pwm.setduty(6, led_g)
                                               pwm.setduty(7, led_b)
                                               curstep = curstep + 1
                                           end
                                       end)

function ledon()
    pwm.setduty(5, led_r)
    pwm.setduty(6, led_g)
    pwm.setduty(7, led_b)
    led_is_on = true
    last_move = tmr.now()
end

function ledoff()
    grad_on = false
    led_is_on = false
    pwm.setduty(5, 0)
    pwm.setduty(6, 0)
    pwm.setduty(7, 0)
end

function calcGradient()
    if grad_time < 5 then
        grad_time = 5
    end
    
    if grad_steps > grad_time then
        grad_steps = grad_time
    end
    
    grad_step_time = grad_time / grad_steps
    if grad_step_time < 5 then
        grad_step_time = 5
        grad_steps = grad_time / grad_step_time
    end
    delta_r = (led_to_r - led_r) / grad_steps
    delta_g = (led_to_g - led_g) / grad_steps
    delta_b = (led_to_b - led_b) / grad_steps
end

function doGradient()
    led_from_r = led_r
    led_from_g = led_g
    led_from_b = led_b
    grad_on = true
    collectgarbage()
    curstep = 1
    calcGradient()
    gradtmr:interval(grad_step_time)
    gradtmr:start()
end

function reverseGradient()
    led_to_r = led_from_r
    led_to_g = led_from_g
    led_to_b = led_from_b
    doGradient()
end

function write_settings(json)
    local jsonf = {}
    if file.open("setting.json", "r") then
        jsonf = sjson.decode(file.read())
        collectgarbage()
        file.close()
    else
         print("Settings file not found! (write_settings)")
    end
    
    for k,v in pairs(json) do jsonf[k] = v end
    
    local ok, jsonw = pcall(sjson.encode, jsonf)
    if ok then
      collectgarbage()
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

    if jm.tocolor ~= nil then
        led_to_r = jm.tocolor.r
        led_to_g = jm.tocolor.g
        led_to_b = jm.tocolor.b
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
            doGradient()
        else
            grad_on = false
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
    collectgarbage()
    processActions(jm)
    write_settings(jm)
  end
end

local function init_settings()
    if file.open("setting.json", "r") then
        local json = sjson.decode(file.read())
        collectgarbage()
        file.close()
        processActions(json)
    else
        print("Settings file not found!")
    end
end

if file.open("private.json", "r") then
    local json = sjson.decode(file.read())
    collectgarbage()
    m = mqtt.Client("nodemcu_rgb_test", 120, json["mqtt_login"], json["mqtt_password"])
    file.close()
end

m:lwt("/lwt", "rgb/kitchen offline", 0, 0)
m:on("connect", function(client) print ("connected") end)
m:on("offline", function(client) print ("offline") end)
m:on("message", onMsg)

if file.open("private.json", "r") then
    local json = sjson.decode(file.read())        
    collectgarbage()
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