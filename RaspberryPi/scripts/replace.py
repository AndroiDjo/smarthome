import sys

filename = sys.argv[1]
strfrom = sys.argv[2]
strto = sys.argv[3]

with open(filename, "r") as f:
    data = f.read().replace(strfrom, strto)
with open(filename, "w") as f:
    f.write(data)