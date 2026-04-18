#!/usr/bin/env python3
"""Chronos DelayEngine performance visualization.

Reads tests/perf_harness/logs/delay_perf.csv (written by
perf_delay_engine_test) and produces a PNG summarizing how much faster
the Chronos SIMD engine is than the naive + JUCE dsp baselines across
block sizes.

Two subplots:
  * Top:    ns/sample (log y). Lower is better.
  * Bottom: realtime factor. Higher is better.

Both include a speedup annotation (chronos vs. each baseline).
"""
import os
import sys

import pandas as pd
import matplotlib.pyplot as plt
import numpy as np

LOG_DIR = "tests/perf_harness/logs"
CSV_PATH = os.path.join(LOG_DIR, "delay_perf.csv")
OUT_PATH = os.path.join(LOG_DIR, "delay_perf.png")

ENGINE_ORDER = ["chronos", "juce_dsp", "naive_scalar"]
ENGINE_COLOR = {
    "chronos":      "#2b8a3e",
    "juce_dsp":     "#1971c2",
    "naive_scalar": "#c92a2a",
}
MODE_PREF = "stereo"  # prefer stereo where multiple modes exist


def main():
    if not os.path.exists(CSV_PATH):
        print(f"CSV not found: {CSV_PATH}. Run perf_delay_engine_test first.", file=sys.stderr)
        sys.exit(1)

    df = pd.read_csv(CSV_PATH)

    # For the headline chart, focus on stereo-vs-stereo, but also capture
    # chronos-mono as a side bar.
    stereo = df[df["mode"] == MODE_PREF].copy()
    mono = df[(df["engine"] == "chronos") & (df["mode"] == "mono")].copy()

    block_sizes = sorted(stereo["block_size"].unique())
    engines = [e for e in ENGINE_ORDER if e in stereo["engine"].unique()]

    fig, axes = plt.subplots(2, 1, figsize=(12, 9), sharex=True)

    # -- Panel 1: ns per sample (log scale). Lower is better. ----------------
    ax0 = axes[0]
    bar_w = 0.8 / (len(engines) + 1)  # +1 for chronos-mono
    x = np.arange(len(block_sizes))
    for i, eng in enumerate(engines):
        values = [
            stereo[(stereo["engine"] == eng) & (stereo["block_size"] == bs)]
            ["ns_per_sample"].mean()
            for bs in block_sizes
        ]
        ax0.bar(x + i * bar_w, values, bar_w,
                label=f"{eng} (stereo)", color=ENGINE_COLOR.get(eng, None),
                edgecolor="black", linewidth=0.5)

    # Chronos-mono overlay
    if not mono.empty:
        mono_values = [
            mono[mono["block_size"] == bs]["ns_per_sample"].mean()
            for bs in block_sizes
        ]
        ax0.bar(x + len(engines) * bar_w, mono_values, bar_w,
                label="chronos (mono)", color="#51cf66",
                edgecolor="black", linewidth=0.5, hatch="//")

    ax0.set_yscale("log")
    ax0.set_ylabel("ns per sample  (log scale, lower is better)")
    ax0.set_title("Chronos DelayEngine vs. baselines — throughput per sample")
    ax0.grid(True, which="both", axis="y", alpha=0.3)
    ax0.legend(loc="upper right")

    # -- Panel 2: realtime factor. Higher is better. --------------------------
    ax1 = axes[1]
    for i, eng in enumerate(engines):
        values = [
            stereo[(stereo["engine"] == eng) & (stereo["block_size"] == bs)]
            ["realtime_factor"].mean()
            for bs in block_sizes
        ]
        ax1.bar(x + i * bar_w, values, bar_w,
                label=f"{eng} (stereo)", color=ENGINE_COLOR.get(eng, None),
                edgecolor="black", linewidth=0.5)

    if not mono.empty:
        mono_values = [
            mono[mono["block_size"] == bs]["realtime_factor"].mean()
            for bs in block_sizes
        ]
        ax1.bar(x + len(engines) * bar_w, mono_values, bar_w,
                label="chronos (mono)", color="#51cf66",
                edgecolor="black", linewidth=0.5, hatch="//")

    ax1.set_ylabel("realtime factor  (higher is better)")
    ax1.set_xlabel("block size (samples)")
    ax1.set_title("One-thread realtime headroom at 48 kHz")
    ax1.grid(True, axis="y", alpha=0.3)
    ax1.set_xticks(x + bar_w * (len(engines) - 1) / 2)
    ax1.set_xticklabels([str(b) for b in block_sizes])
    ax1.legend(loc="upper right")

    # -- Speedup annotations --------------------------------------------------
    # Average speedup of chronos-stereo vs each baseline across all block sizes.
    headline_lines = []
    chronos_ns = stereo[stereo["engine"] == "chronos"].set_index("block_size")["ns_per_sample"]
    for baseline in [e for e in engines if e != "chronos"]:
        base_ns = stereo[stereo["engine"] == baseline].set_index("block_size")["ns_per_sample"]
        common = chronos_ns.index.intersection(base_ns.index)
        if len(common) == 0:
            continue
        speedups = base_ns.loc[common] / chronos_ns.loc[common]
        headline_lines.append(
            f"chronos vs {baseline}: avg {speedups.mean():.2f}x (min {speedups.min():.2f}x, max {speedups.max():.2f}x)"
        )

    if headline_lines:
        ax0.text(0.01, 0.98, "\n".join(headline_lines),
                 transform=ax0.transAxes, va="top", ha="left",
                 fontsize=10,
                 bbox=dict(boxstyle="round", facecolor="white", edgecolor="#888", alpha=0.9))

    plt.tight_layout()
    plt.savefig(OUT_PATH, dpi=150)
    plt.close()
    print(f"Wrote {OUT_PATH}")


if __name__ == "__main__":
    main()
