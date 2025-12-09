import subprocess
#https://docs.platformio.org/en/latest/projectconf/section_env_build.html#projectconf-dynamic-build-flags
revision = (
    subprocess.check_output(["git", "describe"])
    .strip()
    .decode("utf-8")
)
print("-D_V='\"%s\"'" % revision)