import json
import requests
from os import listdir
print("Patching Makefile...")
latest_json = requests.get(
    "https://raw.githubusercontent.com/adrifcastr/NTRDB-Plugin-Host/NTRDBI-Nightlies-Server/update.json").json()
with open("nightly-server/update.json", "w") as updjson:
    latest_json["latest_micro"] = str(int(latest_json["latest_micro"]) + 1)
    updjson.write(json.dumps(latest_json))
with open("Makefile", "r") as f:
    makefile = f.read().replace("0xF8807", "0xF8808")
    makefile = makefile.replace("ICON := meta/icon_3ds.png", "ICON := meta/icon_3ds_night.png")
makefile_list = makefile.split("\n")
for thing in makefile_list:
    if thing.startswith("VERSION_MAJOR"):
        makefile_list[makefile_list.index(thing)] = "VERSION_MAJOR := 0"
    elif thing.startswith("VERSION_MINOR"):
        makefile_list[makefile_list.index(thing)] = "VERSION_MINOR := 0"
    elif thing.startswith("VERSION_MICRO"):
        makefile_list[makefile_list.index(thing)] = "VERSION_MICRO := %s" % latest_json["latest_micro"]
with open("Makefile", "w") as f:
    f.write("\n".join(makefile_list))
print("Patching romfs contents...")
for file in listdir("nightly-server/custom-romfs"):
    with open("romfs/" + file, "wb") as base:
        with open("nightly-server/custom-romfs/" + file, "rb") as new:
            base.write(new.read())
print("Patching update.c...")
with open("source/ui/section/update.c", "r") as f:
    updatec = f.read()
updatec = updatec.replace("NTRDBI-Update-Server", "NTRDBI-Nightlies-Server")
with open("source/ui/section/update.c", "w") as f:
    f.write(updatec)
print("All done!")
