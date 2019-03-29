gpio.mode(3, gpio.OUTPUT)
gpio.write(3, gpio.LOW);
if file.open("private.json", "r") then
    local json = sjson.decode(file.read())
    dofile("init_wifi.lua")(json["wifi_method"],json["ssid"],json["wifipwd"])
    file.close()
end