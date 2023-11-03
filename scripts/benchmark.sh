#!/bin/bash

elapsed_time_counter() {
    local start=$SECONDS
    local elapsed_seconds
    while true; do
        elapsed_seconds=$(( SECONDS - start ))
        printf "\r%-30s %2ds" "Elapsed time:" "$elapsed_seconds"
        sleep 1
    done
}

instances_flag=""
exclude_instances_flag=""
range_instances_flag=""

while [[ $# -gt 0 ]]
do
key="$1"

case $key in
    -i|--instances)
    instances_flag="$2"
    shift # past argument
    shift # past value
    ;;
    -e|--exclude)
    exclude_instances_flag="$2"
    shift # past argument
    shift # past value
    ;;
    -r|--range)
    range_instances_flag="$2"
    shift # past argument
    shift # past value
    ;;
    *)    # unknown option, consider them as positional arguments
    POSITIONAL+=("$1")
    shift # past argument
    ;;
esac
done

# Restore positional parameters
set -- "${POSITIONAL[@]}"

# Get the optional name parameter and current date/time
path_to_tests="${1:-}"
subfolder=$(basename "$path_to_tests")
comment="${2:-}"
solver_path="${3:-}"
solver_name=$(basename "$solver_path")
current_date=$(date "+%Y-%m-%d")
current_time=$(date "+%H-%M-%S")
timeout_seconds="${4:-300}"
custom_command="${5:-}"

# Create the output directories if they don't exist
mkdir -p "out/$current_date/results/$solver_name"
mkdir -p "out/$current_date/logs/$solver_name"

# Create the results and logs files
results_file="${current_time}-results-python-${subfolder}-${comment}-${solver_name}.csv"
logs_file="${current_time}-logs-python-${subfolder}-${comment}-${solver_name}.csv"
echo "Test,Time,Vertices,Edges,Solution" > "out/$current_date/results/$solver_name/$results_file"
echo "Test,Output" > "out/$current_date/logs/$solver_name/$logs_file"

find_command="find $path_to_tests -type f -name \"*.*\""

if [ ! -z "$instances_flag" ]; then
    IFS=',' read -ra instance_numbers <<< "$instances_flag"
    instance_filter=$(printf "_%03d.gr|" "${instance_numbers[@]}")
    instance_filter="${instance_filter%|}"
    find_command="$find_command | grep -E \"(${instance_filter})$\""
fi

if [ ! -z "$exclude_instances_flag" ]; then
    IFS=',' read -ra exclude_instance_numbers <<< "$exclude_instances_flag"
    exclude_instance_filter=$(printf "_%03d.gr|" "${exclude_instance_numbers[@]}")
    exclude_instance_filter="${exclude_instance_filter%|}"
    find_command="$find_command | grep -Ev \"(${exclude_instance_filter})$\""
fi

if [ ! -z "$range_instances_flag" ]; then
    IFS=':' read -ra range_numbers <<< "$range_instances_flag"
    start_range="${range_numbers[0]}"
    end_range="${range_numbers[1]}"
    range_filter=$(seq -f "_%03g.gr" $start_range $end_range | tr '\n' '|')
    range_filter="${range_filter%|}"
    find_command="$find_command | grep -E \"(${range_filter})$\""
fi

printf "%-30s %-10s %-10s %-10s %-10s\n" "Test" "Time" "Vertices" "Edges" "Solution"

# Iterate through all .gr files in tests' subfolders
eval "$find_command" | sort | while read -r test_file; do
    # Get the subfolder and test name
    test_name=$(basename "$test_file")

    # Extract the vertices and edges from the .gr file
    vertices_edges=$(head -n 1 "$test_file" | awk '{print $3 " " $4}')
    vertices=$(echo "$vertices_edges" | cut -d ' ' -f 1)
    edges=$(echo "$vertices_edges" | cut -d ' ' -f 2)

    elapsed_time_counter & elapsed_time_pid=$!
    SECONDS=0
    if [ -z "$custom_command" ]; then
        (timeout $timeout_seconds $solver_path < "$test_file" > temp_python_output.txt)
    else
        (timeout $timeout_seconds bash -c "$custom_command" < "$test_file" > temp_python_output.txt)
    fi    
    elapsed_time=$SECONDS
    kill $elapsed_time_pid 2>/dev/null
    disown $elapsed_time_pid
    printf "\r"

    python_output=$(cat temp_python_output.txt)

    # Extract twin-width value from your C++ program's output
    cpp_solution=$(echo "$python_output" | grep "c twin-width:" | awk '{print $NF}')

    verifier_output=$(python3 verifier.py "$test_file" <(printf "%s" "$python_output") 2>&1)
    verifier_solution=$(echo "$verifier_output" | perl -nle 'print $1 if /Width: (\d+)/')

    # Compare the twin-width values from the C++ program and the verifier
    if [ "$cpp_solution" != "$verifier_solution" ]; then
        echo "WARNING: Discrepancy in twin-width values. C++ program: $cpp_solution, Verifier: $verifier_solution"
    fi

    solution="$verifier_solution"

    # Save the result and log
    echo "$test_name,$elapsed_time,$vertices,$edges,$solution" >> "out/$current_date/results/$solver_name/$results_file"
    echo "$test_name,\"$python_output\"" >> "out/$current_date/logs/$solver_name/$logs_file"

    # Print the elapsed time
    printf "%-30s %-10s %-10s %-10s %-10s\n" "$test_name" "${elapsed_time}s" "$vertices" "$edges" "$solution"

    # Clean up the temporary files
    rm temp_python_output.txt
done

# Create the stats directory if it doesn't exist
mkdir -p "out/$current_date/stats/$solver_name"

# Define the stats file
stats_file="${current_time}-stats-python-${subfolder}-${comment}-${solver_name}.txt"

# Call the Python script to compute the statistics
python3 analyze_solutions.py "out/$current_date/results/$solver_name/$results_file" > "out/$current_date/stats/$solver_name/$stats_file"
