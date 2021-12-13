#!/bin/bash
help()
{
   # Display Help
   echo "Syntax: start_benchmark [-h|e]"
   echo "options:"
   echo "-e     Name of the experiment to run (must be associated with a 'name'.py script)."
   echo
}

while getopts ":he:" option; do
   case $option in
      h) # display Help
         help
         exit;;
      e) # enter exp name
         e_name=$OPTARG;;
     \?) # Invalid option
         echo "Error: Invalid option"
         exit;;
   esac
done

if [ -z "$e_name" ]
then
      help
      exit
fi

python $e_name.py \
		--server_binary /home/ubuntu/aszekeres/Be-Tree/server/dfs-server                      \
		--client_binary /home/ubuntu/aszekeres/Be-Tree/client/dfs-client                      \
		--config_file_directory /home/ubuntu/aszekeres/Be-Tree                                \
		--keys_file /home/ubuntu/aszekeres/Be-Tree/keys.txt                                   \
		--suite_directory /home/ubuntu/aszekeres/dfs_benchmarks                               \
		--logs_directory /tmp/dfs_logs
