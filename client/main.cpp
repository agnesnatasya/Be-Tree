// This test program performs a series of inserts, deletes, updates,
// and queries to a betree.  It performs the same sequence of
// operatons on a std::map.  It checks that it always gets the same
// result from both data structures.

// The program takes 1 command-line parameter -- the number of
// distinct keys it can use in the test.

// The values in this test are strings.  Since updates use operator+
// on the values, this test performs concatenation on the strings.

#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include "betree.hpp"

#include "filesystem.hpp"
#include "common/gflags.hpp"
#include "network/configuration.hpp"
#include "network/fasttransport.hpp"

void timer_start(uint64_t &timer)
{
  struct timeval t;
  assert(!gettimeofday(&t, NULL));
  timer -= 1000000*t.tv_sec + t.tv_usec;
}

void timer_stop(uint64_t &timer)
{
  struct timeval t;
  assert(!gettimeofday(&t, NULL));
  timer += 1000000*t.tv_sec + t.tv_usec;
}

int next_command(FILE *input, int *op, char arg[])
{
  int ret;
  char command[64];

  ret = fscanf(input, "%s %s", command, arg);

  if (ret == EOF)
    return EOF;
  else if (ret != 2) {
    fprintf(stderr, "Parse error\n");
    exit(3);
  }
  
  if (strcmp(command, "Inserting") == 0) {
    *op = 0;
  } else if (strcmp(command, "Updating") == 0) {
    *op = 1;
  } else if (strcmp(command, "Deleting") == 0) {
    *op = 2;
  } else if (strcmp(command, "Query") == 0) {
    *op = 3;
    if (1 != fscanf(input, " -> %s", command)) {
      fprintf(stderr, "Parse error\n");
      exit(3);
    }
  } else if (strcmp(command, "Full_scan") == 0) {
    *op = 4;
  } else if (strcmp(command, "Lower_bound_scan") == 0) {
    *op = 5;
  } else if (strcmp(command, "Upper_bound_scan") == 0) {
    *op = 6;
  } else {
    fprintf(stderr, "Unknown command: %s\n", command);
    exit(1);
  }
  
  return 0;
}

template<class Key, class Value>
void do_scan(typename betree<Key, Value>::iterator &betit,
	     typename std::map<Key, Value>::iterator &refit,
	     betree<Key, Value> &b,
	     typename std::map<Key, Value> &reference)
{
  while (refit != reference.end()) {
    assert(betit != b.end());
    assert(betit.first == refit->first);
    assert(betit.second == refit->second);
    ++refit;
    if (refit == reference.end()) {
      debug(std::cout << "Almost done" << std::endl);
    }
    ++betit;
  }
  assert(betit == b.end());
}

#define DEFAULT_TEST_MAX_NODE_SIZE (1ULL<<6)
#define DEFAULT_TEST_MIN_FLUSH_SIZE (DEFAULT_TEST_MAX_NODE_SIZE / 4)
#define DEFAULT_TEST_CACHE_SIZE (4)
#define DEFAULT_TEST_NDISTINCT_KEYS (1ULL << 10)
#define DEFAULT_TEST_NOPS (1ULL << 12)

void usage(char *name)
{
  std::cout
    << "Usage: " << name << " [OPTIONS]" << std::endl
    << "Tests the betree implementation" << std::endl
    << std::endl
    << "Options are" << std::endl
    << "  Required:"   << std::endl
    << "    -d <backing_store_directory>                    [ default: none, parameter is required ]"           << std::endl
    << "    -m  <mode>  (test or benchmark-<mode>)          [ default: none, parameter required ]"              << std::endl
    << "        benchmark modes:"                                                                               << std::endl
    << "          upserts    "                                                                                  << std::endl
    << "          queries    "                                                                                  << std::endl
    << "  Betree tuning parameters:" << std::endl
    << "    -N <max_node_size>            (in elements)     [ default: " << DEFAULT_TEST_MAX_NODE_SIZE  << " ]" << std::endl
    << "    -f <min_flush_size>           (in elements)     [ default: " << DEFAULT_TEST_MIN_FLUSH_SIZE << " ]" << std::endl
    << "    -C <max_cache_size>           (in betree nodes) [ default: " << DEFAULT_TEST_CACHE_SIZE     << " ]" << std::endl
    << "  Options for both tests and benchmarks" << std::endl
    << "    -k <number_of_distinct_keys>                    [ default: " << DEFAULT_TEST_NDISTINCT_KEYS << " ]" << std::endl
    << "    -t <number_of_operations>                       [ default: " << DEFAULT_TEST_NOPS           << " ]" << std::endl
    << "    -s <random_seed>                                [ default: random ]"                                << std::endl
    << "  Test scripting options" << std::endl
    << "    -o <output_script>                              [ default: no output ]"                             << std::endl
    << "    -i <script_file>                                [ default: none ]"                                  << std::endl;
}

int test(betree<FKey, std::string> &b,
	 uint64_t nops,
	 uint64_t number_of_distinct_keys,
	 FILE *script_input,
	 FILE *script_output)
{
  std::map<FKey, std::string> reference;

  for (unsigned int i = 0; i < nops; i++) {
    int op;
    char ck[256];
    std::string t;
    if (script_input) {
      int r = next_command(script_input, &op, ck);
      t = std::string(ck);
      if (r == EOF)
	exit(0);
      else if (r < 0)
	exit(4);
    } else {
      op = rand() % 7;
      t = std::to_string(rand() % number_of_distinct_keys);
    }
    
    switch (op) {
    case 0: // insert
      if (script_output)
	fprintf(script_output, "Inserting %s\n", t.c_str());
      b.insert(FKey(t), t + ":");
      reference[FKey(t)] = t + ":";
      break;
    case 1: // update
      if (script_output)
	fprintf(script_output, "Updating %s\n", t.c_str());
      b.update(FKey(t), t + ":");
      if (reference.count(FKey(t)) > 0)
        reference[FKey(t)] += t + ":";
      else
        reference[FKey(t)] = t + ":";
      break;
    case 2: // delete
      if (script_output)
	fprintf(script_output, "Deleting %s\n", t.c_str());
      b.erase(FKey(t));
      reference.erase(FKey(t));
      break;
    case 3: // query
      try {
	std::string bval = b.query(FKey(t));
	assert(reference.count(FKey(t)) > 0);
	std::string rval = reference[FKey(t)];
	assert(bval == rval);
	if (script_output)
	  fprintf(script_output, "Query %s -> %s\n", t.c_str(), bval.c_str());
      } catch (std::out_of_range e) {
	if (script_output)
	  fprintf(script_output, "Query %s -> DNE\n", t.c_str());
	assert(reference.count(FKey(t)) == 0);
      }
      break;
    case 4: // full scan
      {
	if (script_output)
	  fprintf(script_output, "Full_scan 0\n");
	auto betit = b.begin();
	auto refit = reference.begin();
	do_scan(betit, refit, b, reference);
      }
      break;
    case 5: // lower-bound scan
      {
	if (script_output)
	  fprintf(script_output, "Lower_bound_scan %s\n", t.c_str());
	auto betit = b.lower_bound(FKey(t));
	auto refit = reference.lower_bound(FKey(t));
	do_scan(betit, refit, b, reference);
      }
      break;
    case 6: // scan
      {
	if (script_output)
	  fprintf(script_output, "Upper_bound_scan %s\n", t.c_str());
	auto betit = b.upper_bound(FKey(t));
	auto refit = reference.upper_bound(FKey(t));
	do_scan(betit, refit, b, reference);
      }
      break;
    default:
      abort();
    }
  }

  std::cout << "Test PASSED" << std::endl;
  
  return 0;
}

void benchmark_upserts(betree<FKey, std::string> &b,
		       uint64_t nops,
		       uint64_t number_of_distinct_keys,
		       uint64_t random_seed)
{
  uint64_t overall_timer = 0;
  for (uint64_t j = 0; j < 100; j++) {
    uint64_t timer = 0;
    timer_start(timer);
    for (uint64_t i = 0; i < nops / 100; i++) {
      uint64_t t = rand() % number_of_distinct_keys;
      b.update(FKey(std::to_string(t)), std::to_string(t) + ":");
    }
    timer_stop(timer);
    printf("%ld %ld %ld\n", j, nops/100, timer);
    overall_timer += timer;
  }
  printf("# overall: %ld %ld\n", 100*(nops/100), overall_timer);
}

void benchmark_queries(betree<FKey, std::string> &b,
		       uint64_t nops,
		       uint64_t number_of_distinct_keys,
		       uint64_t random_seed)
{
  
  // Pre-load the tree with data
  srand(random_seed);
  for (uint64_t i = 0; i < nops; i++) {
    uint64_t t = rand() % number_of_distinct_keys;
    b.update(std::to_string(t), std::to_string(t) + ":");
  }

	// Now go back and query it
  srand(random_seed);
  uint64_t overall_timer = 0;
	timer_start(overall_timer);
  for (uint64_t i = 0; i < nops; i++) {
    uint64_t t = rand() % number_of_distinct_keys;
    b.query(std::to_string(t));
  }
	timer_stop(overall_timer);
  printf("# overall: %ld %ld\n", nops, overall_timer);

}

int main(int argc, char **argv)
{
  char *mode = NULL;
  uint64_t max_node_size = DEFAULT_TEST_MAX_NODE_SIZE;
  uint64_t min_flush_size = DEFAULT_TEST_MIN_FLUSH_SIZE;
  uint64_t cache_size = DEFAULT_TEST_CACHE_SIZE;
  char *backing_store_dir = NULL;
  uint64_t number_of_distinct_keys = DEFAULT_TEST_NDISTINCT_KEYS;
  uint64_t nops = DEFAULT_TEST_NOPS;
  char *script_infile = NULL;
  char *script_outfile = NULL;
  unsigned int random_seed = time(NULL) * getpid();
 
  int opt;
  char *term;
    
  //////////////////////
  // Argument parsing //
  //////////////////////

    gflags::ParseCommandLineFlags(&argc, &argv, true);

    if (FLAGS_configFile == "") {
        fprintf(stderr, "option --configFile is required\n");
        return EXIT_FAILURE;
    }

    if (FLAGS_benchmark == "") {
        fprintf(stderr, "option --benchmark is required\n");
        return EXIT_FAILURE;
    }

    srand(random_seed);

    if (FLAGS_backingStoreDir == "") {
        fprintf(stderr, "option --backingStoreDir is required\n");
        return EXIT_FAILURE;
    }

    if (FLAGS_clientIP == "") {
        fprintf(stderr, "option --clientIP is required\n");
        return EXIT_FAILURE;
    }

    // Load configuration
    std::ifstream configStream(FLAGS_configFile);
    if (configStream.fail()) {
        fprintf(stderr, "unable to read configuration file: %s\n", FLAGS_configFile.c_str());
    }
    network::Configuration config(configStream);

    network::FastTransport *transport = new network::FastTransport(config,
                                                FLAGS_clientIP,
                                                //FLAGS_numServerThreads,
                                                1,
                                                4,
                                                0,
                                                0,
                                                0);

    StorageClient client(config, transport);


  ////////////////////////////////////////////////////////
  // Construct a betree and run the tests or benchmarks //
  ////////////////////////////////////////////////////////
  
  one_file_per_object_backing_store ofpobs(FLAGS_backingStoreDir);
  swap_space sspace(&ofpobs, &client, cache_size);
  betree<FKey, std::string> b(&sspace, max_node_size, min_flush_size);
  //betree<std::string, std::string> b(&sspace, max_node_size, min_flush_size);
  //betree<uint64_t, std::string> b(&sspace, max_node_size, min_flush_size);

  if (FLAGS_benchmark == "benchmark-upserts")
    benchmark_upserts(b, nops, number_of_distinct_keys, random_seed);
  else if (FLAGS_benchmark ==  "benchmark-queries")
    benchmark_queries(b, nops, number_of_distinct_keys, random_seed);

  return 0;
}

