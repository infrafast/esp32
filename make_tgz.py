Import("env")
import re
import subprocess
import shutil
from pathlib import Path
import sys

# === CONFIGURATION ===
DEST_FOLDER = Path("/Volumes/web/fwupdate")

# === Helpers ===
def _normalize(v: str) -> str:
    v = str(v).strip()
    v = v.replace("\\", "")
    v = v.strip("'\"")
    return v

def _try_from_cppdefines() -> str | None:
    for d in env.get("CPPDEFINES", []):
        if isinstance(d, tuple) and d[0] == "_V":
            return _normalize(d[1])
        if isinstance(d, str) and d == "_V":
            return "1"
    return None

def _try_from_flags_regex() -> str | None:
    sources = []

    try:
        sources.append(str(env.GetProjectOption("build_flags") or ""))
    except Exception:
        pass

    sources.append(str(env.get("BUILD_FLAGS", "")))

    for k in ("CCFLAGS", "CXXFLAGS", "CPPFLAGS"):
        v = env.get(k)
        if isinstance(v, (list, tuple)):
            sources.append(" ".join(map(str, v)))
        elif v:
            sources.append(str(v))

    blob = "\n".join(sources)

    m = re.search(r"-D_V=(\".*?\"|'.*?'|[^\s]+)", blob)
    if not m:
        return None

    return _normalize(m.group(1))

def get_version_or_die() -> str:
    v = _try_from_cppdefines()
    if v:
        return v

    v = _try_from_flags_regex()
    if v:
        return v

    print("Erreur : _V non défini (introuvable côté script)")
    print("DEBUG CPPDEFINES =", env.get("CPPDEFINES"))
    print("DEBUG BUILD_FLAGS =", env.get("BUILD_FLAGS"))
    sys.exit(1)

# === Action principale ===
def make_tgz(*args, **kwargs):
    version = get_version_or_die()

    project_dir = Path(env["PROJECT_DIR"])
    data_dir = project_dir / "data"

    if not data_dir.exists():
        print(f"Erreur : dossier introuvable -> {data_dir}")
        sys.exit(1)

    output_file = Path(f"{version}.tgz")

    cmd = [
        "tar",
        "--format=ustar",
        "--exclude=.DS*",
        "-czvf",
        str(output_file),
        "-C",
        str(data_dir),
        "."
    ]

    print("Commande exécutée :")
    print(" ".join(cmd))

    r = subprocess.run(cmd)
    if r.returncode != 0:
        sys.exit(r.returncode)

    # --- Déplacement de l'archive .tgz vers DEST_FOLDER ---
    DEST_FOLDER.mkdir(parents=True, exist_ok=True)
    tgz_dest = DEST_FOLDER / output_file.name
    shutil.move(str(output_file), str(tgz_dest))
    print(f"Archive déplacée vers : {tgz_dest}")        

    # --- Déplacement du firmware .bin.gz vers DEST_FOLDER avec version ---
    progname = env.subst("$PROGNAME")
    data_dir = Path(env["PROJECT_DIR"]) / "data"
    fw_gz = data_dir / f"{progname}.bin.gz"

    if fw_gz.exists():
        DEST_FOLDER.mkdir(parents=True, exist_ok=True)

        fw_dest = DEST_FOLDER / f"{progname}.{version}.bin.gz"
        shutil.move(str(fw_gz), str(fw_dest))

        print(f"Firmware déplacé vers : {fw_dest}")
    else:
        print(f"Aucun firmware .bin.gz à déplacer dans {data_dir}")



# Hook fiable après compilation
env.AddPostAction("buildprog", make_tgz)
