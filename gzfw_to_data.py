Import("env")

import gzip
import shutil
from pathlib import Path
import sys


def compress_firmware(source, target, env):
    print("Compressing firmware with gzip...")

    build_dir = Path(env.subst("$BUILD_DIR"))
    progname = env.subst("$PROGNAME")

    source_file = build_dir / f"{progname}.bin"

    if not source_file.exists():
        print(f"Erreur : firmware introuvable -> {source_file}")
        sys.exit(1)

    # Dossier data du projet
    data_dir = Path(env["PROJECT_DIR"]) / "data"
    data_dir.mkdir(parents=True, exist_ok=True)

    dest_file = data_dir / f"{progname}.bin.gz"

    with source_file.open("rb") as f_in, gzip.open(dest_file, "wb", compresslevel=9) as f_out:
        shutil.copyfileobj(f_in, f_out)

    orig_size = source_file.stat().st_size
    gz_size = dest_file.stat().st_size

    print(
        "Compression OK : {:.1f}% ({} → {} bytes)".format(
            (gz_size / orig_size) * 100,
            orig_size,
            gz_size,
        )
    )
    print(f"Gzip généré : {dest_file}")


# 🔒 Hook fiable : juste avant upload (le firmware existe déjà)
#env.AddPreAction("upload", compress_firmware)

# Exécution fiable après la compilation du firmware
env.AddPostAction("buildprog", compress_firmware)
