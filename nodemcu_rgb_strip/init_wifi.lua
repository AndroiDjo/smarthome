return function (m,i,p)
local cfg={}
local firstconnect = true
cfg.ssid=i
cfg.pwd = string.len(p)>=8 and p or nil
wifi.setmode(wifi.STATION)
wifi.nullmodesleep(false)
wifi.sta.config(cfg)
wifi.eventmon.register(wifi.eventmon.STA_CONNECTED,function(T)
tmr.create(0):alarm(3000,tmr.ALARM_SINGLE,function()
print("IP:"..wifi.sta.getip())
if firstconnect then
firstconnect = false
dofile("init_mqtt.lua")
end
end)
end)
end
