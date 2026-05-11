Import("env")
import os

def patch_redirect(source, target, env):
    path = os.path.join(
        env.subst("$PROJECT_LIBDEPS_DIR"),
        env["PIOENV"],
        "ESP Async WebServer/src/WebRequest.cpp"
    )
    
    if not os.path.exists(path):
        print("⚠️ AsyncWebServer WebRequest.cpp not found, skipping patch")
        return
    
    with open(path, "r") as f:
        content = f.read()
    
    old = (
        "void AsyncWebServerRequest::redirect(const String& url){\n"
        "  AsyncWebServerResponse * response = beginResponse(302);\n"
        "  response->addHeader(\"Location\",url);\n"
        "  send(response);\n"
        "}"
    )
    new = (
        "void AsyncWebServerRequest::redirect(const String& url){\n"
        "  AsyncWebServerResponse * response = beginResponse(302);\n"
        "  if (!response) return;\n"
        "  response->addHeader(\"Location\",url);\n"
        "  send(response);\n"
        "}"
    )
    
    if old in content:
        with open(path, "w") as f:
            f.write(content.replace(old, new))
        print("✅ AsyncWebServer redirect() patched")
    elif "if (!response) return;" in content:
        print("✅ AsyncWebServer already patched, skipping")
    else:
        print("⚠️ AsyncWebServer patch target not found — check manually")

env.AddPreAction("buildprog", patch_redirect)
