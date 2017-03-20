try:
    from base64 import b64encode
    from os import getenv
    from json import dumps
    from subprocess import check_output
    import requests
    itemstopush = {
        "output/NTRDBI.zip": "NTRDBI.zip",
        "output/3ds-arm/NTRDBI.cia": "latest.cia",
        "nightly-server/update.json": "update.json"
    }
    version = str(
        check_output('git log -n 1 --pretty=format:"%h"', shell=True), 'utf-8')
    github = "https://api.github.com/repos/adrifcastr/NTRDB-Plugin-Host/"
    branch = "NTRDBI-Nightlies-Server"

    def upload(file, filename):
        fileurl = github + "contents/%s"
        print("Uploading", file, "as", filename, end=" ")
        reqjs = {"message": "File:%s Commit:%s" % (filename, version),
                 "committer": {
                 "name": "Nightly Builds",
                 "email": "noreply@octonezd.pw"
        },
            "branch": branch,
        }
        with open(file, 'rb') as f:
            reqjs["content"] = str(b64encode(f.read()), 'utf-8')
        tree = requests.get(github + "git/trees/heads/" + branch).json()
        for file in tree["tree"]:
            if file["path"] == filename:
                reqjs["sha"] = file["sha"]
                break
        req = dumps(reqjs)
        headers = {"Authorization": "token " + getenv("GITHUBAUTH")}
        r = requests.put(fileurl % (filename), headers=headers, data=req)
        if r.status_code == 201 or r.status_code == 200:
            print("[OK]")
        else:
            print("[FAIL:", r.status_code, "]", sep="")

    for file in itemstopush:
        upload(file, itemstopush[file])
except Exception as e:
    print("An error occured during uploading nightly build.")
    print(
        "To pretend secure data leaks, no error info was displayed")
    print("Contact t.me/OctoNezd if problem persists")
    raise SystemExit
