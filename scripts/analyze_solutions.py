import pandas as pd
from scipy.stats import gmean
import argparse
import numpy as np


def compute_stats(file_path, num_instances=None):
    # Read the CSV file
    data = pd.read_csv(file_path)

    # Filter out rows where Solution is empty
    if num_instances is None:
        completed_data = data[data['Solution'].notna()]
    else:
        completed_data = data[data['Solution'].notna()].head(num_instances)

    # Replace 0 with 0.1 in the 'Solution' column to make the geometric mean more meaningful
    completed_data['Solution'] = completed_data['Solution'].replace(0, 0.1)

    # Compute various statistics of the Solution column
    geo_mean = gmean(completed_data['Solution'])
    arith_mean = completed_data['Solution'].mean()
    median = completed_data['Solution'].median()
    std_dev = completed_data['Solution'].std()
    variance = completed_data['Solution'].var()
    min_val = completed_data['Solution'].min()
    max_val = completed_data['Solution'].max()
    percentiles = np.percentile(completed_data['Solution'], [25, 50, 75])

    print(f"The geometric mean of the solutions is: {geo_mean}")
    print(f"The arithmetic mean of the solutions is: {arith_mean}")
    print(f"The median of the solutions is: {median}")
    print(f"The standard deviation of the solutions is: {std_dev}")
    print(f"The variance of the solutions is: {variance}")
    print(f"The minimum solution size is: {min_val}")
    print(f"The maximum solution size is: {max_val}")
    print(f"The 25th, 50th, and 75th percentiles of the solutions are: {percentiles}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Compute various statistics of solutions')
    parser.add_argument('file_path', type=str, help='Path to the CSV file')
    parser.add_argument('--num_instances', type=int, default=None, help='Number of instances to consider (optional)')
    args = parser.parse_args()

    compute_stats(args.file_path, args.num_instances)
