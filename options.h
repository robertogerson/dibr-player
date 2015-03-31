#include <map>
#include <string>
#include <getopt.h>

static std::map <char, std::string> opts;

static struct option long_options[] =
{
  /* These options don't set a flag.
     We distinguish them by their indices. */
  {"fullscreen",  no_argument,        0, 'f'},
  {"opencl",      no_argument,        0, 'o'},
  {"stereo",      no_argument,        0, 's'},
  {"width",       required_argument,  0, 'w'},
  {"output",      required_argument,  0, 'u'},
  {"height",      required_argument,  0, 'h'},
  {"width",       required_argument,  0, 'w'},
  {"headtracking",no_argument,  0, 't'},
  {0, 0, 0, 0}
};

void parse_opts(int argc, char **argv)
{
  int c;
  int option_index;

  while (1)
  {
    c = getopt_long(argc, argv, "h:w:fosi:d:u:",
                    long_options, &option_index);

    /* detect end of the options */
    if (c == -1)
      break;

    switch (c)
    {
      case 'h':
        printf ("Run with h = %s.\n", optarg);
        opts['h'] = optarg;
        break;
      case 'w':
        printf ("Run with w = %s.\n", optarg);
        opts['w'] = optarg;
      break;
      case 'i':
        printf ("Input file found: %s\n", optarg);
        opts['i'] = optarg;
        break;
      case 'd':
        printf ("Depth file found: %s\n", optarg);
        opts['d'] = optarg;
        break;
      case 'u':
        printf ("Output file: %s\n", optarg);
        opts['u'] = optarg;
        break;
      case 'f':
        printf ("Use fullscreen.\n");
        opts['f'] = "1";
        break;
      case 'o':
        printf ("Run with opencl support.\n");
        opts['o'] = "1";
        break;
      case 's':
        printf ("Run with stereo support.\n");
        opts['s'] = "1";
      case 't':
        printf ("Run with head tracking support.\n");
        opts['t'] = "1";
      default:
      abort();
    }
  }
}

