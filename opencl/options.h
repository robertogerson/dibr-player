#include <map>
#include <string>
#include <getopt.h>

static std::map <char, std::string> opts;

static struct option long_options[] =
{
  /* These options set a flag. */
  {"help",     no_argument,       0, 'h'},
  /* These options don't set a flag.
        We distinguish them by their indices. */
  {"opencl",  no_argument,        0, 'o'},
  {"stereo",  no_argument,        0, 's'},
  {"input",   required_argument,  0, 'i'},
  {0, 0, 0, 0}
};

void parse_opts(int argc, char **argv)
{
  int c;
  int option_index;

  while (1)
  {
    c = getopt_long(argc, argv, "hosi:",
                    long_options, &option_index);

    /* detect end of the options */
    if (c == -1)
      break;

    switch (c)
    {
      case 'h':
        printf ("TODO: print_help.\n");
        opts['h'] = "1";
        break;
      case 'i':
        printf ("Input file found: %s\n", optarg);
        opts['i'] = optarg;
        break;
      case 'o':
        printf ("Run with opencl support.\n");
        opts['o'] = "1";
        break;
      case 's':
        printf ("Run with stereo support.\n");
        opts['s'] = "1";
      default:
      abort();
    }
  }
}
