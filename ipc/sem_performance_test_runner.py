import subprocess
import psutil
import time
import os


def calculate_cpu_usage(usage):
    num_cpu = len(usage[0])

    # Get sum for each CPU
    col_summary = []
    for i in range(num_cpu):
        column = [row[i] for row in usage]
        col_summary.append(sum(column))

    # Get avg for each CPU
    for i in range(num_cpu):
        col_summary[i] = col_summary[i] / len(usage)

    # Return avg for CPU
    return sum(col_summary)/num_cpu

buffer_size = 50
consumers = 125
producers = 66
consumer_time = 0.001
producer_time = 0.002

# Print configuration
print(f"Running with buffer_size: {buffer_size}\tconsumers: {consumers}/{consumer_time}s\tproducers:{producers}/{producer_time}s")

# Define cmd
current_dir = os.path.dirname(__file__)
command = f"{current_dir}/sem_test -s -bs {buffer_size} -cl 100 -c {consumers} -p {producers} -cs {consumer_time} -ps {producer_time}"

# Start process & get PID
process = subprocess.Popen(command, shell=True)
pid = process.pid

# Monitor CPU and time
cpu_usage = []
start_time = time.time()
while process.poll() is None:
    cpu_percent = psutil.cpu_percent(interval=1, percpu=True)
    cpu_usage.append(cpu_percent)
end_time = time.time()

# Calculate CPU usage and time
avg_cpu_usage = calculate_cpu_usage(cpu_usage)
time_taken = end_time - start_time

# Print results
print(f"Avg. CPU usage: {avg_cpu_usage:.2f}\tRun time: {time_taken:.2f}s")


