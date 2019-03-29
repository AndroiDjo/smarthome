if file.open("private.json", "r") then
    local json = sjson.decode(file.read())
    m = mqtt.Client("esp_light_lobby", 120, json["mqtt_login"], json["mqtt_password"])
    file.close()
end
m:lwt("/lwt", "offline", 0, 0)
m:on("connect", function(client) print ("connected") end)
m:on("offline", function(client) print ("offline") end)
m:on("message", function(client, topic, data) 
  if data ~= nil then
    if data == "1" then
      gpio.write(3, gpio.LOW);
    elseif data == "0" then
      gpio.write(3, gpio.HIGH);
    end
  end
end)

m:on("overflow", function(client, topic, data)
  print(topic .. " partial overflowed message: " .. data )
end)

if file.open("private.json", "r") then
    local json = sjson.decode(file.read())
    m:connect(json["mqtt_host"], json["mqtt_port"], 0, function(client)
      print("connected")
      client:subscribe("light/lobby", 0)
      client:subscribe("light/all", 0)
    end,
    function(client, reason)
      print("failed reason: " .. reason)
    end)
    file.close()
end
