#!/usr/bin/env python3
"""Build and verify memory usage for a list of nRF52 PlatformIO environments."""
import argparse
import json
import sys
from memory_checker import build_and_check


def main():
    parser = argparse.ArgumentParser(description="Verify nRF52 memory thresholds")
    parser.add_argument("envs", nargs="+", help="PlatformIO environment names")
    parser.add_argument("--ram-max", type=float, default=80.0, help="RAM threshold %%")
    parser.add_argument("--flash-max", type=float, default=90.0, help="Flash threshold %%")
    args = parser.parse_args()

    results = []
    for env in args.envs:
        print(f"\n=== Building {env} ===", file=sys.stderr)
        report = build_and_check(env, ram_max=args.ram_max, flash_max=args.flash_max)
        results.append(report)
        if report["stats"]:
            print(
                f"RAM:   {report['stats']['ram_percent']}% "
                f"({report['stats']['ram_used']}/{report['stats']['ram_total']})",
                file=sys.stderr,
            )
            print(
                f"Flash: {report['stats']['flash_percent']}% "
                f"({report['stats']['flash_used']}/{report['stats']['flash_total']})",
                file=sys.stderr,
            )
        for err in report["errors"]:
            print(f"ERROR: {err}", file=sys.stderr)
        if not report["build_ok"]:
            print(f"BUILD FAILED for {env}", file=sys.stderr)

    print(json.dumps(results, indent=2))


if __name__ == "__main__":
    main()
