#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "harper.h"

static int fix = 0;
static int number = 0;
static int verbosity = 0;
static char* comment = "";
static char* delimiter = "";

static char* input_file = "-";
static char* output_file = "-";
static char* suggestion_file = "-";

void
help(char* progname)
{
	printf("Usage: %s [OPTION] [FILE]\n", progname);
	puts("Perform grammar checking with 'harper'.");
	puts("");
	puts("With no FILE, or when FILE is -, read standard input.");
	puts("");
	puts("Mandatory options to long options are mandatory for short options too.");
	puts("  -c, --comment=COMMENT       ignore lines starting with any character in COMMENT");
	puts("  -d, --delimiter=DELIMITER   start suggestion lines with DELIMITER (default: '')");
	puts("  -f, --fix-file              automatically apply suggestions");
	puts("  -n, --number-lines          provide line:col number for each suggestion");
	puts("  -o, --document-output=FILE  output document text to FILE");
	puts("  -O, --suggestion-output=FILE");
	puts("                              output suggestions to FILE");
	puts("  -q, --quiet                 do not print suggestions");
	puts("  -s, --silent                do not output anything");
	puts("  -h, --help                  print this help message and exit");
	puts("  -v, --version               print version information and exit");
	puts("");
	printf("Try 'man %s' for more information.\n", progname);
}

void
version(char* progname)
{
	printf("%s v1.1.0\n", progname);
	printf("harper-c v%s\n", harper_get_lib_version());
	printf("harper-core v%s\n", harper_get_core_version());
	puts("");
	puts("Copyright (C) 2024 Jack Renton Uteg.");
	puts("License GPLv3+: GNU GPL version 3 or later <https://gnu.org/licenses/gpl.html>.");
	puts("This is free software: you are free to change and redistribute it.");
	puts("There is NO WARRANTY, to the extent permitted by law.");
	puts("");
	puts("Written by Jack R. Uteg.");
}

// Top Answer to Stack Overflow 122616
// Used under CC-BY-SA 4.0 as per Stack Overflow T&C
char*
strip(char* string)
{
	char* end;

	while (isspace((unsigned char) *string)) string++;

	if (*string == 0)
		return string;

	end = string + strlen(string) - 1;
	while (end > string && isspace((unsigned char) *end)) end--;

	end[1] = '\0';
	return string;
}

FILE*
check_and_open(char* progname, char* filename, int read)
{
	struct stat st;

	if (strcmp(filename, "-"))
	{
		if (stat(filename, &st) && read)
		{
			fprintf(stderr, "%s: %s: %s\n", progname, filename, strerror(errno));
			return NULL;
		}
		if (S_ISDIR(st.st_mode))
		{
			fprintf(stderr, "%s: %s: Is a directory\n", progname, filename);
			return NULL;
		}
		if (! S_ISREG(st.st_mode) && read)
		{
			fprintf(stderr, "%s: %s: Not a regular file\n", progname, filename);
			return NULL;
		}
		return fopen(filename, read ? "r" : "w");
	}
	else
	{
		if (read)
			return stdin;
		else
			return stdout;
	}
}

int
main(int argc, char* argv[])
{
	char* progname = basename(argv[0]);

	int char_code;

	FILE* input;
	FILE* doc_output;
	FILE* sug_output;

	int line_number;
	int mistakes = 0;

	char* line;
	size_t line_length;

	Document* line_document;
	LintGroup* line_linter;
	int line_lint_count;
	Lint** line_lints;
	char* lint_message;

	int start;
	int end;

	char* SHORTOPTS = "c:d:fno:O:qshv";
	static struct option LONGOPTS[] =
	{
		/* Set flags. */
		{"fix", no_argument, &fix, 1},
		{"number", no_argument, &number, 1},
		{"quiet", no_argument, &verbosity, 1},
		{"silent", no_argument, &verbosity, 2},
		/* No flag-setting. */
		{"comment", required_argument, NULL, 'c'},
		{"delimiter", required_argument, NULL, 'd'},
		{"document-output", required_argument, NULL, 'o'},
		{"suggestion-output", required_argument, NULL, 'O'},
		{"help", no_argument, NULL, 'h'},
		{"version", no_argument, NULL, 'v'},
		{0, 0, 0, 0}
	};

	while ((char_code = getopt_long(argc, argv, SHORTOPTS, LONGOPTS, NULL)) != -1)
	{
		switch (char_code)
		{
			case 0:
				/* We've set a flag. */
				break;
			case 'c':
				comment = optarg;
				break;
			case 'd':
				delimiter = optarg;
				break;
			case 'f':
				fix = 1;
				break;
			case 'n':
				number = 1;
				break;
			case 'o':
				output_file = optarg;
				break;
			case 'O':
				suggestion_file = optarg;
				break;
			case 'q':
				verbosity = 1;
				break;
			case 's':
				verbosity = 2;
				break;
			case 'h':
				help(progname);
				return 0;
			case 'v':
				version(progname);
				return 0;
		}
	}

	if (argc - optind)
	{
		input_file = argv[optind];
		optind++;
	}

	//We only take one file; use `cat` if you want to do >1 at once.
	if (argc - optind)
	{
		fprintf(stderr, "%s: %s: Extra operand\n", progname, argv[optind + 2]);
		return 1;
	}

	if ((input = check_and_open(progname, input_file, 1)) == NULL)
		return 127;
	if ((doc_output = check_and_open(progname, output_file, 0)) == NULL)
		return 127;
	if ((sug_output = check_and_open(progname, suggestion_file, 0)) == NULL)
		return 127;

	line_number = 1;
	mistakes = 0;
	while (getline(&line, &line_length, input) > 0)
	{
		if (verbosity <= 1)
			fprintf(doc_output, "%s", line); // Newline always included =P

		if ((line_document = harper_create_document(strip(line))) == NULL)
		{
			fprintf(stderr, "%s: Failed to create document\n", progname);
			return 127;
		}

		// Create a lint group
		if ((line_linter = harper_create_lint_group()) == NULL)
		{
			fprintf(stderr, "%s: Failed to create lint group\n", progname);
			harper_free_document(line_document);
			return 127;
		}

		// Get and print lints
		if ((line_lints = harper_get_lints(line_document, line_linter, &line_lint_count)) != NULL)
		{
			for (int i = 0; i < line_lint_count; i++)
			{
				if ((lint_message = harper_get_lint_message(line_lints[i])) != NULL)
				{
					start = harper_get_lint_start(line_lints[i]);
					end = harper_get_lint_end(line_lints[i]);
					if (!verbosity)
					{
						if (number)
							fprintf(sug_output, "%s%u:%u '%.*s': %s\n", delimiter, line_number, start + 1, end - start, line + start, lint_message);
						else
							fprintf(sug_output, "%s'%.*s': %s\n", delimiter, end - start, line + start, lint_message);
					}
					free(lint_message);
					mistakes++;
				}
			}
			harper_free_lints(line_lints, line_lint_count);
		}

		// Clean up
		harper_free_lint_group(line_linter);
		harper_free_document(line_document);

		line_number++;
	}
	return mistakes ? 1 : 0;
}
