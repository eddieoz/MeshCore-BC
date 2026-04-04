import re
import subprocess
import sys


def parse_memory_usage(text: str):
    """Parse PlatformIO memory usage summary from build output."""
    ram_match = re.search(
        r"RAM:\s+\[[=\s]+\]\s+([0-9.]+)%\s+\(used\s+(\d+)\s+bytes\s+from\s+(\d+)\s+bytes\)",
        text,
    )
    flash_match = re.search(
        r"Flash:\s+\[[=\s]+\]\s+([0-9.]+)%\s+\(used\s+(\d+)\s+bytes\s+from\s+(\d+)\s+bytes\)",
        text,
    )

    if not ram_match or not flash_match:
        return None

    return {
        "ram_percent": float(ram_match.group(1)),
        "ram_used": int(ram_match.group(2)),
        "ram_total": int(ram_match.group(3)),
        "flash_percent": float(flash_match.group(1)),
        "flash_used": int(flash_match.group(2)),
        "flash_total": int(flash_match.group(3)),
    }


def check_memory_thresholds(stats: dict, ram_max: float = 80.0, flash_max: float = 90.0):
    """Return a list of error strings if memory usage exceeds thresholds."""
    errors = []
    if stats["ram_percent"] > ram_max:
        errors.append(
            f"RAM usage {stats['ram_percent']}% exceeds threshold {ram_max}% "
            f"({stats['ram_used']}/{stats['ram_total']} bytes)"
        )
    if stats["flash_percent"] > flash_max:
        errors.append(
            f"Flash usage {stats['flash_percent']}% exceeds threshold {flash_max}% "
            f"({stats['flash_used']}/{stats['flash_total']} bytes)"
        )
    return errors


def build_and_check(env: str, ram_max: float = 80.0, flash_max: float = 90.0):
    """Run pio build for the given environment and check memory thresholds."""
    result = subprocess.run(
        ["pio", "run", "-e", env],
        capture_output=True,
        text=True,
    )
    combined = result.stdout + result.stderr
    stats = parse_memory_usage(combined)
    if stats is None:
        return {
            "env": env,
            "ok": False,
            "errors": ["Could not parse memory usage from build output"],
            "stats": None,
            "build_ok": result.returncode == 0,
        }
    errors = check_memory_thresholds(stats, ram_max, flash_max)
    return {
        "env": env,
        "ok": len(errors) == 0 and result.returncode == 0,
        "errors": errors,
        "stats": stats,
        "build_ok": result.returncode == 0,
    }


if __name__ == "__main__":
    env = sys.argv[1] if len(sys.argv) > 1 else None
    if not env:
        print("Usage: python scripts/memory_checker.py <pio_environment>")
        sys.exit(1)
    report = build_and_check(env)
    print(f"Environment: {report['env']}")
    if report["stats"]:
        print(
            f"RAM:   {report['stats']['ram_percent']}% "
            f"({report['stats']['ram_used']}/{report['stats']['ram_total']})"
        )
        print(
            f"Flash: {report['stats']['flash_percent']}% "
            f"({report['stats']['flash_used']}/{report['stats']['flash_total']})"
        )
    for err in report["errors"]:
        print(f"ERROR: {err}")
    sys.exit(0 if report["ok"] else 1)
