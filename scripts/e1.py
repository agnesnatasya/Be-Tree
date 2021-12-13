from collections import defaultdict
from dfs_benchmarks import (
    Parameters, ParametersAndResult, parse_args, run_suite, haas_clients,
    haas_servers)
from sheets_query import header
import benchmark


"""
    Experiment 1.
"""

def main(args):
    # Set benchmark parameters.
    base_parameters = Parameters(
        config_file_directory=args.config_file_directory,
        keys_file=args.keys_file,
        num_keys=None,
        num_server_threads=None,
        server_binary=args.server_binary,
        client_binary=args.client_binary,
        benchmark_duration_seconds=80,
        benchmark_warmup_seconds=35,
        write_percentage=0,
        zipf_coefficient=None,
        num_client_machines=None,
        num_clients_per_machine=1,
        num_threads_per_client=None,
        num_fibers_per_client_thread=None,
        suite_directory=args.suite_directory,
        logs_directory=args.logs_directory,
    )

    parameters_list = [
      base_parameters._replace(
          num_server_threads = num_server_threads,
          num_keys = num_keys,
          num_client_machines = num_client_machines,
          num_threads_per_client = num_threads_per_client,
          num_fibers_per_client_thread = num_fibers_per_client_thread,
          zipf_coefficient = zipf_coefficient,
      )

      # Keep load 24 clients per core
      for (num_server_threads,
           num_keys,
           num_client_machines,
           num_threads_per_client,
           num_fibers_per_client_thread) in [
                                       (1, 1 * 500000, 1, 1, 1),
                                       ]

      for zipf_coefficient in [0]
    ]

    # Run every experiment three times.
    #parameters_list = [q for p in parameters_list for q in [p] * 3]

    # Run the suite.
    suite_dir = benchmark.SuiteDirectory(args.suite_directory, 'e1')
    suite_dir.write_dict('args.json', vars(args))
    run_suite(suite_dir, parameters_list, haas_clients(), haas_servers())

if __name__ == '__main__':
    main(parse_args())
