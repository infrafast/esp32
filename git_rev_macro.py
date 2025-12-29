import subprocess
#https://docs.platformio.org/en/latest/projectconf/section_env_build.html#projectconf-dynamic-build-flags
revision = subprocess.check_output(["git", "describe", "--tags", "--always"]).strip().decode("utf-8")
print(f'-D_V=\\"{revision}\\"')
