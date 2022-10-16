#include "getopt.h"

#include <cstdlib>
#include <cstring>

bool _optreset = true;
int optind = 0;
char* optarg = nullptr;

static optparser globalParser;
int getopt_long(int argc, char** argv, char* /* optstring */, const option opts[], int* /* longindex */) {
	if(_optreset) {
		globalParser.reset(argc, argv, opts);
		optind = 0;
		optarg = nullptr;
		_optreset = false;
	}

	int ret = globalParser.next();
	optind = globalParser.get_index();
	optarg = globalParser.get_arg();
	return ret;
}

#ifdef TEST_GETOPT
#include <cstdio>
int wmain(int argc, char** argv) {
	static option opts[]{
		{L"a_first", L'a', false},
		{L"b_second", L'b', false},
		{L"c_witharg", L'c', true},
		{nullptr, 0, false},
	};

	int optind = 0;
	char* optarg = nullptr;
	while(int o = getopt_long(argc, argv, opts, &optind, &optarg)) {
		if(o == -1) {
			fwprintf(stderr, L"Done with argument parsing.\r\n");
			break;
		}
		fwprintf(stderr, L"Got option `%c' at index %d with argument `%ls'\r\n", o, optind, optarg);
	}

	fwprintf(stderr, L"\r\nArguments (* = after optind)\r\n----\r\n");
	for(int i = 0; i < argc; ++i) {
		fwprintf(stderr, L"%c %ls\r\n", i >= optind ? L'*' : L' ', argv[i]);
	}

	return 0;
}
#endif
