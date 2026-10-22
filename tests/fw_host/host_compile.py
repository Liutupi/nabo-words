"""Build the real game/renderer host, using CC on Windows or POSIX."""
import os
from pathlib import Path
import shlex
import subprocess

ROOT = Path(__file__).resolve().parents[2]
MAIN = ROOT / "firmware/main"


def build_host(out: Path) -> Path:
    out = out.resolve()
    out.mkdir(parents=True, exist_ok=True)
    value = os.environ.get("CC", "cc")
    command = [value] if Path(value).is_file() else shlex.split(value)
    if Path(command[0]).stem == "zig" and len(command) == 1:
        command.append("cc")
    exe = out / ("host.exe" if os.name == "nt" else "host")
    sources = [ROOT / "tests/fw_host/host_main.c", *sorted(MAIN.glob("nw_*.c")),
               *sorted((MAIN / "words").glob("*.c"))]
    libs = ["-lws2_32"] if os.name == "nt" else []
    subprocess.run([*command, "-O1", "-std=gnu11", "-Wall", "-Werror", f"-I{MAIN}",
                    "-o", str(exe), *map(str, sources), *libs], check=True)
    return exe
