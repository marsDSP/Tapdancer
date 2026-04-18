import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
import os

def plot_delay_alignment(csv_path):
    if not os.path.exists(csv_path):
        print(f"File not found: {csv_path}")
        return
    
    df = pd.read_csv(csv_path)
    output_path = csv_path.replace(".csv", ".png")
    
    plt.figure(figsize=(12, 6))
    
    # Plot performance (time taken) vs offset for different num_samples
    sns.lineplot(data=df, x='offset', y='time_ms', hue='num_samples', marker='o')
    plt.title("DelayEngine SIMD Alignment: Performance vs Offset")
    plt.ylabel("Time taken (ms)")
    plt.xlabel("Buffer Offset (samples)")
    plt.grid(True, alpha=0.3)
    
    # Add a text label for correctness
    max_err = df['max_error'].max()
    plt.annotate(f"Max Alignment Error: {max_err:.2e}", 
                 xy=(0.05, 0.95), xycoords='axes fraction', 
                 bbox=dict(boxstyle="round", fc="w", alpha=0.8),
                 color='green' if max_err < 1e-5 else 'red')

    plt.savefig(output_path, dpi=150)
    plt.close()
    print(f"Generated {output_path}")

if __name__ == "__main__":
    log_dir = "tests/simd_harness/logs"
    plot_delay_alignment(os.path.join(log_dir, "simd_alignment_delay.csv"))
