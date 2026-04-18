import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
import os

def plot_math_alignment(csv_path):
    if not os.path.exists(csv_path):
        print(f"File not found: {csv_path}")
        return
    
    df = pd.read_csv(csv_path)
    output_path = csv_path.replace(".csv", ".png")
    
    plt.figure(figsize=(12, 6))
    
    # Plot error vs offset for different functions
    sns.barplot(data=df, x='function', y='max_error', hue='offset')
    plt.title("FasterMath SIMD Alignment: Correctness vs Offset")
    plt.ylabel("Max Error (SIMD vs Scalar)")
    plt.xlabel("Function")
    plt.grid(True, alpha=0.3, axis='y')
    plt.yscale('symlog', linthresh=1e-10)
    
    plt.savefig(output_path, dpi=150)
    plt.close()
    print(f"Generated {output_path}")

if __name__ == "__main__":
    log_dir = "tests/simd_harness/logs"
    plot_math_alignment(os.path.join(log_dir, "simd_alignment_math.csv"))
