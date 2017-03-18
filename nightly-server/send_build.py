try:
    from base64 import b64encode
    from os import getenv
    from json import dumps
    from subprocess import check_output
    import requests
    with open("output/NTRDBI.zip", 'rb') as f:
        content = str(b64encode(f.read()), 'utf-8')
    version = str(
        check_output('git log -n 1 --pretty=format:"%h"', shell=True), 'utf-8')
    req = dumps(
        {"message": "[AUTO]NTRDBI Nightly build file for %s" % version,
          "committer": {
            "name": "AutoNightly",
            "email": "noreply@octonezd.pw"
          },
         "branch": "NTRDBI-Nightlies-Server",
         "content": content
         }
    )
    headers = {"Authorization": "token " + getenv("GITHUBAUTH")}
    fileurl = "https://api.github.com/repos/adrifcastr/NTRDB-Plugin-Host/contents/%s"
    r = requests.put(fileurl % (version + ".zip"), headers=headers, data=req)
    print("GitHub returned", r.status_code)
except Exception as e:
    print("An error occured during uploading nightly build.")
    print(
        "To pretend secure data leaks, no error info was displayed")
    print("Contact t.me/OctoNezd if problem persists")
    raise SystemExit
