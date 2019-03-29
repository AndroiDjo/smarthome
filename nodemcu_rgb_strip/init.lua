gpio.mode(5, gpio.OUTPUT)
gpio.mode(6, gpio.OUTPUT)
gpio.mode(7, gpio.OUTPUT)
gpio.write(5, gpio.LOW)
gpio.write(6, gpio.LOW)
gpio.write(7, gpio.LOW)
if file.open("private.json", "r") then
    local json = sjson.decode(file.read())        
    dofile("init_wifi.lua")(json["wifi_method"],json["ssid"],json["wifipwd"])
    file.close()
end