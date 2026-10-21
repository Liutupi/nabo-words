"""把《NABO 单词护照》独立固件刷进 AI Passport 工牌（Windows / macOS / Linux 都能用）。

先整片备份工牌原来的 8 MB Flash 到 backups/，再写入引导、分区表、程序和资源（esptool 写完会逐块校验）。
要恢复原来的固件：python -m esptool --chip esp32c3 write-flash 0x0 backups/<备份文件>.bin

用法：python flash_words.py [COM 口]      （不写就自动找 Espressif 的 USB 口）
      python flash_words.py COM8 --app    只更新程序和资源，不备份、保留学习记录
"""
import datetime
import hashlib
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
LOG = open(HERE / "flash_log.txt", "a", encoding="utf-8")


def say(*parts) -> None:
    text = " ".join(str(p) for p in parts)
    print(text, flush=True)
    LOG.write(text + "\n")
    LOG.flush()
IMAGES = [(0x0, "bootloader.bin"), (0x8000, "partition-table.bin"), (0x10000, "nabo-words-passport.bin"),
          (0x210000, "assets.bin")]


def find_port() -> str:
    from serial.tools import list_ports
    ports = [p for p in list_ports.comports() if p.vid == 0x303A]
    if len(ports) != 1:
        raise SystemExit(f"找到 {len(ports)} 个 Espressif USB 口，请在命令后面写上 COM 口，例如 python flash_words.py COM8")
    return ports[0].device


def esptool(port: str, *args: str) -> str:
    cmd = [sys.executable, "-m", "esptool", "--chip", "esp32c3", "--port", port, "--baud", "921600", *args]
    say(">", " ".join(cmd[2:]))
    r = subprocess.run(cmd, capture_output=True, text=True, encoding="utf-8", errors="replace")
    say(r.stdout[-1500:], r.stderr[-1500:])
    if r.returncode:
        say(f"失败：esptool 出错（{r.returncode}），已停止")
        raise SystemExit(1)
    return r.stdout


def main() -> None:
    say("==== 开始", datetime.datetime.now().isoformat(timespec="seconds"))
    for _, name in IMAGES:
        if not (HERE / name).exists():
            raise SystemExit(f"缺少 {name}")
    args = [a for a in sys.argv[1:] if not a.startswith("--")]
    app_only = "--app" in sys.argv
    port = args[0] if args else find_port()
    info = esptool(port, "--after", "no-reset", "flash-id")
    if "8MB" not in info:
        raise SystemExit("不是 8 MB Flash 的工牌，已停止")
    images = IMAGES
    if app_only:
        images = [i for i in IMAGES if i[0] >= 0x10000]
    else:
        mac = next((l.split("MAC:")[1].strip() for l in info.splitlines() if "MAC:" in l), "unknown").replace(":", "")
        backups = HERE / "backups"
        backups.mkdir(exist_ok=True)
        stamp = datetime.datetime.now().strftime("%Y%m%d-%H%M%S")
        backup = backups / f"passport-{mac}-before-words-{stamp}.bin"
        esptool(port, "--after", "no-reset", "read-flash", "0x0", "0x800000", str(backup))
        data = backup.read_bytes()
        if len(data) != 0x800000:
            raise SystemExit("备份大小不对，已停止，没有写入")
        say(f"备份完成：{backup.name}  sha256={hashlib.sha256(data).hexdigest()}")
        esptool(port, "--after", "no-reset", "erase-region", "0x200000", "0x10000")   # 学习记录分区清空
    cmd = ["--after", "hard-reset", "write-flash", "--flash-mode", "dio", "--flash-freq", "80m", "--flash-size", "8MB"]
    for off, name in images:
        cmd += [hex(off), str(HERE / name)]
    esptool(port, *cmd)
    say("完成：刷写完成，工牌会自动重启进入 NABO 单词护照。")


if __name__ == "__main__":
    try:
        main()
    except SystemExit as e:
        if e.code not in (None, 0, 1):
            say("失败：", e.code)
        raise
