try:
    from base64 import b64encode
    from os import getenv
    from json import dumps
    import requests
    with open("output/NTRDBI.zip", 'rb') as f:
        content = b64encode(f.read())
    req = dumps(
        {"message": "[AUTO]NTRDBI Nightly build updated.",
         "branch": "NTRDBI-Nightlies-Server",
         "commiter": {
             "name": "NTRDBI Nightly Builds"
         },
         "content": content
         }
    )
    headers = {"Authorization", "token " + getenv("GITHUBAUTH")}
    fileurl = "https://api.github.com/repos/adrifcastr/NTRDB-Plugin-Host/contents/%s"
    r = requests.put(fileurl % ("NTRDBI.zip"), headers=headers)
except Exception:
    print("An error occured during uploading nightly build.")
    print(
        "To pretend secure data leaks, no error info was displayed")
    print("Contact t.me/OctoNezd if problem persists")
    raise SystemExit
