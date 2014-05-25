#include <map>
#include <string>
#include <getopt.h>

static std::map <char, std::string> opts;

static struct option long_options[] =
{
  /* These options don't set a flag.
     We distinguish them by their indices. */
  {"help",     no_argument,       0, 'h'},
  {"fullscreen", no_argument,     0, 'f'},
  {"opencl",  no_argument,        0, 'o'},
  {"stereo",  no_argument,        0, 's'},
  {"input",   required_argument,  0, 'i'},
  {"resolution",   required_argument,   0, 'r'},
  {0, 0, 0, 0}
};

void parse_opts(int argc, char **argv)
{
  int c;
  int option_index;

  while (1)
  {
    c = getopt_long(argc, argv, "hfosi:",
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
      case 'f':
        printf ("Use fullscreen.\n");
        opts['f'] = "1";
        break;
      case 'o':
        printf ("Run with opencl support.\n");
        opts['o'] = "1";
        break;
      case 'e':
        printf ("Run with stereo support.\n");
        opts['e'] = "1";
      case 's':
        printf ("Run with width == %s.\n", optarg);
        opts['s'] = optarg;
        break;
      case 'r':
        printf ("Run with resolution == %s.\n", optarg);
        opts['r'] = optarg;
        break;
      default:
      abort();
    }
  }
}
