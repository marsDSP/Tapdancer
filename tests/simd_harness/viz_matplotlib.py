import pandas as pd
import matplotlib.pyplot as plt
import os
import glob
import numpy as np

def plot_trig_test(csv_path):
    df = pd.read_csv(csv_path)
    test_name = os.path.basename(csv_path).replace("_results.csv", "").replace("simd_", "")
    output_path = csv_path.replace(".csv", ".png")
    
    # Identify columns
    x_col = 'x'
    # For trig tests, we usually have std_func, pade_scalar, pade_simd, abs_err_scalar, abs_err_simd
    # For tanh, we might have more.
    # For boundtopi, it's different.
    
    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(12, 10), gridspec_kw={'height_ratios': [2, 1]})
    
    if 'std_sin' in df.columns or 'std_cos' in df.columns or 'std_tan' in df.columns or 'std_tanh' in df.columns:
        std_col = [c for c in df.columns if c.startswith('std_')][0]
        ax1.plot(df[x_col], df[std_col], label='Standard (Ref)', color='black', alpha=0.5, linewidth=2)
        
        simd_col = 'pade_simd'
        if simd_col not in df.columns:
            # try to find any SIMD result column
            simd_cols = [c for c in df.columns if 'simd' in c and 'err' not in c and 'diff' not in c]
            if simd_cols: simd_col = simd_cols[0]
            
        if simd_col in df.columns:
            ax1.plot(df[x_col], df[simd_col], label='SIMD Pade', color='blue', linestyle='--', alpha=0.8)
        
        # We use diff_simd_scalar for PASS/FAIL (SIMD Correctness)
        # and abs_err_simd for the error plot (Approximation accuracy)
        err_col = 'abs_err_simd'
        if err_col not in df.columns:
             err_cols = [c for c in df.columns if 'err_simd' in c]
             if err_cols: err_col = err_cols[0]
        
        correctness_col = 'diff_simd_scalar'
        if correctness_col not in df.columns:
            correctness_col = err_col # fallback if new column missing
            
        max_correctness_err = df[correctness_col].max()
        pass_status = max_correctness_err < 1e-6 # Strict for SIMD matching Scalar
        
        max_approx_err = df[err_col].max()
    elif 'scalar_res' in df.columns: # boundToPi
        ax1.plot(df[x_col], df['scalar_res'], label='Scalar (Ref)', color='black', alpha=0.5)
        ax1.plot(df[x_col], df['simd_res'], label='SIMD', color='blue', linestyle='--')
        err_col = 'abs_diff'
        max_correctness_err = df[err_col].max()
        pass_status = max_correctness_err < 1e-5
        max_approx_err = max_correctness_err
    else:
        print(f"Unknown format for {csv_path}")
        return

    ax1.set_title(f"SIMD Test: {test_name.upper()}")
    ax1.legend()
    ax1.grid(True, alpha=0.3)
    
    # Error plot (showing approximation error vs standard)
    ax2.plot(df[x_col], df[err_col], color='blue', alpha=0.6, label='Approx Error (vs std)')
    if 'diff_simd_scalar' in df.columns:
         ax2.plot(df[x_col], df['diff_simd_scalar'], color='green' if pass_status else 'red', 
                  label='SIMD-Scalar Diff', linewidth=1, alpha=0.8)
    
    ax2.set_yscale('log')
    ax2.set_ylabel("Error (log scale)")
    ax2.legend(loc='upper right', fontsize='small')
    ax2.grid(True, which="both", ls="-", alpha=0.2)
    
    # Status indicator
    color = 'green' if pass_status else 'red'
    status_text = "PASSED" if pass_status else "FAILED"
    
    info_text = f"Status: {status_text}\nSIMD Correctness Error: {max_correctness_err:.2e}"
    if max_approx_err != max_correctness_err:
        info_text += f"\nMax Approximation Error: {max_approx_err:.2e}"
        
    fig.text(0.5, 0.02, info_text, 
             ha='center', fontsize=12, fontweight='bold', color=color,
             bbox=dict(facecolor='white', edgecolor=color, boxstyle='round,pad=0.5'))

    plt.tight_layout(rect=[0, 0.05, 1, 1])
    plt.savefig(output_path, dpi=150)
    plt.close()
    print(f"Generated {output_path} - {status_text}")

def plot_delay_test(csv_path):
    df = pd.read_csv(csv_path)
    output_path = csv_path.replace(".csv", ".png")
    modes = df['mode'].unique()

    fig, axes = plt.subplots(len(modes), 2, figsize=(18, 6 * len(modes)))
    if len(modes) == 1:
        axes = [axes]

    overall_pass = True
    for i, mode in enumerate(modes):
        mode_data = df[df['mode'] == mode]
        max_err = mode_data['error'].abs().max()
        pass_status = max_err < 1e-5
        if not pass_status: overall_pass = False
        
        color = 'green' if pass_status else 'red'

        # Signal Plot
        axes[i][0].plot(mode_data['sample'], mode_data['scalar_out'], label='Scalar (Ref)', color='black', alpha=0.4)
        axes[i][0].plot(mode_data['sample'], mode_data['simd_out'], label='SIMD', color='blue', linestyle='--', alpha=0.7)
        axes[i][0].set_title(f"{mode} - Signals")
        axes[i][0].legend()
        axes[i][0].grid(True, alpha=0.3)

        # Error Plot
        axes[i][1].plot(mode_data['sample'], mode_data['error'], color=color)
        axes[i][1].set_title(f"{mode} - Error (Max: {max_err:.2e})")
        axes[i][1].grid(True, alpha=0.3)
        
        # Pass/Fail label in plot
        axes[i][1].text(0.95, 0.95, "PASS" if pass_status else "FAIL", 
                        transform=axes[i][1].transAxes, ha='right', va='top',
                        fontsize=12, fontweight='bold', color='white',
                        bbox=dict(facecolor=color, alpha=0.8))

    status_color = 'green' if overall_pass else 'red'
    status_text = "ALL PASSED" if overall_pass else "SOME FAILED"
    fig.suptitle(f"DelayEngine SIMD Correctness - {status_text}", fontsize=16, fontweight='bold', color=status_color)
    
    plt.tight_layout(rect=[0, 0.03, 1, 0.95])
    plt.savefig(output_path, dpi=150)
    plt.close()
    print(f"Generated {output_path} - {status_text}")

if __name__ == "__main__":
    log_dir = "tests/simd_harness/logs"
    
    # Delay Engine
    delay_csv = os.path.join(log_dir, "simd_delay_data.csv")
    if os.path.exists(delay_csv):
        plot_delay_test(delay_csv)
        
    # Trig tests
    trig_csvs = glob.glob(os.path.join(log_dir, "simd_*_results.csv"))
    for csv in trig_csvs:
        plot_trig_test(csv)
