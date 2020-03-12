import re
import sys

filename = sys.argv[1]
newfile = sys.argv[2]

reg = re.compile('[^а-яА-ЯёЁ ]')
print(reg.sub('', s))

with open(newfile, "a") as out:
    with open(filename, "r") as f:
        for line in f:
            newline = reg.sub('', line).replace("  ", " ")
            out.write(newline+"\n")