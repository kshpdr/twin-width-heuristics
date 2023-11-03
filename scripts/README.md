# Scripts for Benchmarking and Analysis

This directory contains a collection of scripts used for benchmarking the solver, verifying solutions, and analyzing results.

## Files:

1. `benchmark.sh`: A shell script to run the solver on a set of test instances.
2. `verifier.py`: A Python script provided by the PACE challenge to verify the solutions produced by the solver.
3. `analyze_solutions.py`: A Python script to analyze the results of the benchmark and generate statistics.

## Using `benchmark.sh`:

To run a benchmark:

```
./benchmark.sh <path to test folder> "<your comment>" <path to executable> <timelimit secs>
```


**Flags available:**
- `-i`: Include instances with a certain number (assumes instance names are formatted like *_number.gr).
- `-e`: Exclude certain instances.
- `-r`: Specify a range of instances, e.g., 1:100 to include instances from 1 to 100.

Only exact instances are provided in this repository due to the large size of heuristic instances. Public instance can be found [here](https://cloudtcs.tcs.uni-luebeck.de/index.php/s/QMxJFWgDZF4bEo2) and private [here](https://cloudtcs.tcs.uni-luebeck.de/index.php/s/PPpJsy6J3XGipHP)

**Example usage:**
```
./benchmark.sh ../tests/exact/ "Test solver on exact instances" ../src/main 300 -r 1:100 -e 10,15,30
```


After running the benchmark, an `out/` folder is created containing:
- Results: Twin-width values and time taken.
- Logs: Actual solutions produced by the solver.
- Statistics: Various metrics about the run.

## Using `verifier.py`:

The `verifier.py` script is used by `benchmark.sh` to verify solutions. If the verifier yields a different value than the benchmark script, a warning is displayed.

To run the verifier manually:

```
python3 verifier.py <path to test> <path to solution>
```

Or, to input a solution directly in the terminal:

```
python3 verifier.py <path to test> <(printf "<your solution is here>")
```


## Using `analyze_solutions.py`:

This script is automatically run after `benchmark.sh` completes its execution. It generates statistics about the test run and saves them in the `out/date/stats` subfolder.

To run the script manually:

```
python3 analyze_solutions.py <path to csv file in results folder>
```