import json, math

irCommandsPath = 'ircommands.json'
partlength = 16

def sendCommand(mqtt, commandname, topic):
    with open(irCommandsPath, 'r') as f:
        ircommands = json.load(f)
        if commandname in ircommands:
            command = ircommands[commandname]
            commandlen = len(command)
            partsCount = math.ceil(commandlen / partlength)
            arrayIndex = 0
            for i in range(partsCount):
                commandpart = {}
                commandpart['split'] = []
                if i == 0:
                    commandpart['rawsize'] = commandlen
                elif i == partsCount - 1:
                    commandpart['rawend'] = True
                for j in range(partlength):
                    if arrayIndex >= commandlen:
                        break
                    commandpart['split'].append(command[arrayIndex])
                    arrayIndex += 1
                mqtt.publish(topic, json.dumps(commandpart))
