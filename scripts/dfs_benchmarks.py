from collections import namedtuple
from process_logs import BenchmarkResult, process_client_logs
from pyrem.host import RemoteHost
from pyrem.task import Parallel
import argparse
import benchmark
import csv
import datetime
import time
import json
import os
import os.path
import subprocess
import time

# A set of benchmark parameters.
Parameters = namedtuple('Parameters', [
    # Client and server parameters. ############################################
    # The directory in which the config files are stored.
    'config_file_directory',
    # The text file that contains the keys that are preloaded in the tree.
    'keys_file',
    # The number of keys to read from the key file.
    'num_keys',
    # The number of server threads.
    'num_server_threads',
    # The server binary.
    'server_binary',
    # The client binary.
    'client_binary',
    # The number of seconds that the clients run (must be bigger than
    # 2*benchmark_warmup_seconds).
    'benchmark_duration_seconds',
    # The number of seconds that the clients warmup.
    'benchmark_warmup_seconds',
    # The percentage of operations in a transaction that are writes.
    'write_percentage',
    # The zipfian coefficient used to select keys.
    'zipf_coefficient',
    # The number of client machines on which we run clients. Can be between 1
    # and 5 since we have 5 client machines.
    'num_client_machines',
    # The number of clients to run on every machine. There are
    # `num_client_machines * num_clients_per_machine` total clients.
    'num_clients_per_machine',
    # The number of threads to run on every client.
    'num_threads_per_client',
    # The number of fibers (micro threads) to run inside each client thread
    # (to utilize the core more efficiently).
    'num_fibers_per_client_thread',
    # The directory into which benchmark results are written. If the output
    # directory is foo/, then output is written to a directory foo/1247014124
    # where 1247014124 is a unique id for the benchmark.
    'suite_directory',
    # The directory into which log files are being are written.
    'logs_directory',
])


# ParametersAndResult is a concatenation of a Parameters (the input) and a
# BenchmarkResult (the output).
ParametersAndResult = namedtuple(
    'ParametersAndResult',
    Parameters._fields + BenchmarkResult._fields)


def boxed(s):
    msg = '# {} #'.format(s)
    return '\n'.join(['#' * len(msg), msg, '#' * len(msg)])


# Remote clients and hosts.

# The phy_port identifies the index of the active NIC port to use
# to send dataplane traffic.
def haas_clients():
    return {
        RemoteHost('10.172.209.107') : {'phys_port'  : 0}, # sc2-hs2-b1637
    }

def haas_servers():
    return {
        RemoteHost('10.172.209.99') : {'phys_port'  : 0}, # sc2-hs2-b1629 
    }

def run_benchmark(bench_dir, clients, servers, parameters):
    # Clear the clients' and servers' out and err files in /mnt/log.
    print(boxed('Clearing *_out.txt and *_err.txt'))
    clear_out_files = Parallel([host.run(['rm', parameters.logs_directory + '/*_out.txt'])
                                for host in list(clients.keys()) + list(servers.keys())],
                                aggregate=True)
    clear_out_files.start(wait=True)
    clear_err_files = Parallel([host.run(['rm', '/tmp/dfs_logs/*_err.txt'])
                                for host in list(clients.keys()) + list(servers.keys())],
                                aggregate=True)
    clear_err_files.start(wait=True)

    # Clear the clients' log files in /mnt/log.
    print(boxed('Clearing *.log'))
    clear_log_files = Parallel([client.run(['rm', '/tmp/dfs_logs/*.log'])
                                for client in list(clients.keys())],
                                aggregate=True)
    clear_log_files.start(wait=True)

    bench_dir.write_string('logs_cleared_time.txt', str(datetime.datetime.now()))

    # Start the servers.
    print(boxed('Starting servers.'))
    server_tasks = []
    # command to enable core dump on the server in case of SIGSEGV
    core_dump_cmd = [
        "ulimit -c unlimited;",
        "sudo sysctl -w kernel.core_pattern=/tmp/core-%e.%p.%h.%t; ",
    ]
    # command to increase the number of fds we can open
    max_open_files_cmd = [
        "ulimit -n 16384;",
    ]
    # command to free buff/cache
    drop_caches_cmd = [
        "sudo bash -c \"echo 3 > /proc/sys/vm/drop_caches\"; "
    ]
    for (server_index, server) in enumerate(sorted(list(servers.keys()), key=lambda h: h.hostname)):
        cmd = [
            "sudo",
            parameters.server_binary,
            "--configFile", os.path.join(parameters.config_file_directory, 'config.txt'),
            "--serverIndex", str(server_index),
            "--keysFile", parameters.keys_file,
            "--numKeys", str(parameters.num_keys),
            "--numShards", "1",
            "--shardIndex", "0",
            "--numServerThreads", str(parameters.num_server_threads),
        ]

        # We capture the stdout and stderr of the servers using the trick
        # outlined in [1]. pyrem has some support for capturing stdout and
        # stderr as well, but it doesn't alway work.
        #
        # [1]: https://stackoverflow.com/a/692407/3187068
        cmd.append(('> >(tee /tmp/dfs_logs/server_{0}_out.txt) ' +
                    '2> >(tee /tmp/dfs_logs/server_{0}_err.txt >&2)')
                   .format(server.hostname))

        # Record (and print) the command we run, so that we can re-run it later
        # while we debug.
        print('Running {} on {}.'.format(' '.join(core_dump_cmd + max_open_files_cmd + cmd), server.hostname))
        bench_dir.write_string(
            'server_{}_cmd.txt'.format(server.hostname),
            ' '.join(cmd) + '\n')

        server_tasks.append(server.run(drop_caches_cmd + core_dump_cmd + max_open_files_cmd + cmd))
    parallel_server_tasks = Parallel(server_tasks, aggregate=True)
    parallel_server_tasks.start()

    # Wait for the servers to start.
    print(boxed('Waiting for servers to start.'))
    time.sleep(1 + 2 * parameters.num_server_threads)
    #time.sleep(170)
    bench_dir.write_string('servers_started_time.txt', str(datetime.datetime.now()))

    # Start the clients and wait for them to finish.
    print(boxed('Starting clients at {}.'.format(datetime.datetime.now())))
    seconds = int(time.time()) # total seconds that passed since unix epoch
    client_tasks = []
    for host_i, client in enumerate(list(clients.keys()[:parameters.num_client_machines])):
        for client_i in range(parameters.num_clients_per_machine):
            cmd = [
                "ulimit -n 8192;" , # increase how many fds we can open
                "sudo",
                parameters.client_binary,
                "--configFile", os.path.join(parameters.config_file_directory, 'config.txt'),
                "--keysFile", parameters.keys_file,
                "--numKeys", str(parameters.num_keys),
                "--backingStoreDir", os.path.join(parameters.suite_directory, 'backend_store'),
                "--numShards", "1",
                "--duration", str(parameters.benchmark_duration_seconds),
                "--warmup", str(parameters.benchmark_warmup_seconds),
                "--wPer", str(parameters.write_percentage),
                "--benchmark", "benchmark-upserts",
                #"--replScheme", str(parameters.repl_scheme),
                "--numServerThreads", str(parameters.num_server_threads),
                "--numClientThreads", str(parameters.num_threads_per_client),
                "--numClientFibers", str(parameters.num_fibers_per_client_thread),
                "--clientIP", str(client.hostname),
                "--physPort", str(clients[client]['phys_port'])
            ]

            # As with the servers, we record the stdout and stderr of the
            # clients. See above for details.
            cmd.append(('> >(tee /tmp/dfs_logs/client_{0}_{1}_out.txt) ' +
                        '2> >(tee /tmp/dfs_logs/client_{0}_{1}_err.txt >&2)')
                       .format(client.hostname, client_i))

            # Record (and print) the command we run, so that we can re-run it
            # later while we debug.
            print('Running {} on {}.'.format(' '.join(cmd), client.hostname))
            bench_dir.write_string(
                'client_{}_{}_cmd.txt'.format(client.hostname, client_i),
                ' '.join(cmd) + '\n')

            client_tasks.append(client.run(cmd, quiet=True))
    parallel_client_tasks = Parallel(client_tasks, aggregate=True)
    parallel_client_tasks.start(wait=True)
    bench_dir.write_string('clients_done_time.txt', str(datetime.datetime.now()))

    # Copy stdout and stderr files over.
    print(boxed('Copying *_out.txt and *_err.txt.'))
    for host in list(servers.keys()) + list(clients.keys()):
        subprocess.call([
            'scp',
            '{}:/tmp/dfs_logs/*_out.txt'.format(host.hostname),
            bench_dir.path
        ])
        subprocess.call([
            'scp',
            '{}:/tmp/dfs_logs/*_err.txt'.format(host.hostname),
            bench_dir.path
        ])

    # Copy the client logs over.
    print(boxed('Copying *.log.'))
    for client in list(clients.keys()[:parameters.num_client_machines]):
        subprocess.call([
            'scp',
            '{}:/tmp/dfs_logs/*.log'.format(client.hostname),
            bench_dir.path
        ])

    bench_dir.write_string('logs_copied_time.txt', str(datetime.datetime.now()))

    # Concatenate client logs.
    print(boxed('Concatenating and processing logs.'))
    subprocess.call([
        'cat {0}/*.log > {0}/client.log'.format(bench_dir.path)
    ], shell=True)

    # Process logs.
    # TODO: process logs
#    try:
#        results = process_client_logs(
#            '{}/client.log'.format(bench_dir.path),
#            warmup_sec=parameters.benchmark_warmup_seconds,
#            duration_sec=parameters.benchmark_duration_seconds -
#                         2*parameters.benchmark_warmup_seconds)
#        bench_dir.write_dict('results.json', results._asdict())
#    except ValueError:
    results = None

    bench_dir.write_string('logs_processed_time.txt', str(datetime.datetime.now()))

    # Kill the servers.
    print(boxed('Killing servers.'))
    #time.sleep(50)
    # We can't use PyREM's stop function because we need sudo priviledges
    #parallel_server_tasks.stop()
    kill_tasks = []
    for (server_index, server) in enumerate(sorted(list(servers.keys()), key=lambda h: h.hostname)):
        cmd = [
            "sudo pkill dfs-server",
        ]

        # Record (and print) the command we run, so that we can re-run it later
        # while we debug.
        print('Running {} on {}.'.format(' '.join(cmd), server.hostname))
        kill_tasks.append(server.run(cmd))
    parallel_kill_tasks = Parallel(kill_tasks, aggregate=True)
    parallel_kill_tasks.start(wait=True)

    bench_dir.write_string('servers_killed_time.txt', str(datetime.datetime.now()))

    return results

def run_suite(suite_dir, parameters_list, clients, servers):
    # Store the list of parameters.
    parameters_strings = '\n'.join(str(p) for p in parameters_list)
    suite_dir.write_string('parameters_list.txt', parameters_strings)

    # Incrementally write out benchmark results.
    with suite_dir.create_file('results.csv') as f:
        csv_writer = csv.writer(f)
        csv_writer.writerow(ParametersAndResult._fields)

        # Run benchmarks.
        for (i, parameters) in enumerate(parameters_list, 1):
            with suite_dir.new_benchmark_directory() as bench_dir:
                print(boxed('Running experiment {} / {} in {}'
                            .format(i, len(parameters_list), bench_dir.path)))
                bench_dir.write_dict('parameters.json', parameters._asdict())
                bench_dir.write_string('parameters.txt', str(parameters))

                result = run_benchmark(bench_dir, clients, servers, parameters)
                if result:
                    csv_writer.writerow(ParametersAndResult(*(parameters + result)))
                    f.flush()

def parser():
    # https://stackoverflow.com/a/4028943/3187068
    home_dir = os.path.expanduser('~')

    parser = argparse.ArgumentParser(
        formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument(
        '--server_binary',
        type=str,
        required=True,
        help='The server binary.')
    parser.add_argument(
        '--client_binary',
        type=str,
        required=True,
        help='The client benchmark binary.')
    parser.add_argument(
        '--config_file_directory',
        type=str,
        default=os.path.join(home_dir, 'dfs_benchmarks'),
        help='The directory in which the config files are stored.')
    parser.add_argument(
        '--keys_file',
        type=str,
        default=os.path.join(home_dir, 'dfs_benchmarks/keys.txt'),
        help='The keys file.')
    parser.add_argument(
        '--suite_directory',
        type=str,
        default=os.path.join(home_dir, 'dfs_benchmarks/tmp'),
        help='The directory into which benchmark results are written.')
    parser.add_argument(
        '--logs_directory',
        type=str,
        default=os.path.join(home_dir, 'dfs_benchmarks/tmp/dfs_logs'),
        help='The directory into which logs are written.')
    return parser


def parse_args():
    args = parser().parse_args()

    # Sanity check command line arguments.
    assert os.path.exists(args.config_file_directory)
    assert os.path.isfile(args.keys_file)
    assert os.path.exists(args.suite_directory)
    assert os.path.exists(args.logs_directory)

    return args

if __name__ == '__main__':
    main(parse_args())
