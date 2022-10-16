#include "optparser.h"

#include <cstdlib>
#include <cstring>

void optparser::reset(int argc, char ** argv, const option opts[]) {
	_argc = argc;
	_argv = argv;
	_lastarg = 1;
	_optind = 1; // skip argv[0], progname
	_optpos = 0;
	_optarg = nullptr;

	_options = &opts[0];
}

int optparser::next() {
	int idx = _optind;
	if(!_optpos) {
		// no option position (later, if we hit the end of the pack, we clear it)
		for(; idx < _argc && _argv[idx][0] != L'-'; ++idx);
	}

	if(idx == _argc) {
		_optarg = nullptr;
		return -1; // no more args, we're done!
	}

	char* arg = _argv[idx];
	bool longopt = arg[1] == L'-';

	const option* foundopt = nullptr;
	char* foundarg = nullptr;
	int npos = 1; // number of positions occupied by args and options

	if(!_optpos && !longopt) _optpos = 1;

	for(const option* o = _options; o->name; ++o) {
		bool found = false;
		if(!longopt && arg[_optpos] == o->shopt) {
			found = true;
			if(o->has_arg && arg[_optpos + 1] != L'\0') {
				// special case: argument specified right after option
				foundarg = arg + _optpos + 1;
				_optpos = -1; // incremented later, to 0
			}
		} else if(longopt) {
			char* eq = strchr(arg, L'=');
			int arglen = eq ? eq - arg - 2 : strlen(arg + 2); // 2: length of --
			if(
				strlen(o->name) != arglen // lengths don't match
				|| strncmp(arg + 2, o->name, arglen) != 0 // or text doesn't match
				) continue; // no match

			if(eq) {
				foundarg = eq + 1;
			}
			found = true;
		} else {
			continue; // no match
		}

		if(found) {
			if(o->has_arg && !foundarg) {
				int argpos = idx + 1;
				if(argpos >= _argc) break; // option missing arg: opterr?

				++npos; // Consume another slot
				foundarg = _argv[argpos];
			}

			foundopt = o;
			break;
		}
	}

	_optpos += static_cast<int>(!longopt); // increment only if short opt
	_optind = idx;

	if(!_optpos || arg[_optpos] == L'\0') {
		_optpos = 0;
		if(idx != _lastarg) {
			// if this option isn't adjacent to the last one,
			// move it down so it is.

			char* save[2];
			memmove(save, _argv + idx, npos * sizeof(char*));
			// 12 34 -s 56 78 -r val
			// |la   |idx         npos=1
			// |S |D              (memmove Src, Dst)
			// |-----|            (memmove Len)
			// later,
			// -s 12 34 56 78 -r val
			//    |la         |idx npos=2
			//    |S    | D        Src, Dst
			//    |-----------|    Len
			memmove(_argv + _lastarg + npos, _argv + _lastarg, (idx - _lastarg) * sizeof(char*));
			memmove(_argv + _lastarg, save, npos * sizeof(char*));
		}
		_lastarg += npos;
		_optind = _lastarg;
	}

	_optarg = foundarg;

	if(longopt && arg[2] == L'\0') {
		return -1;
	}

	return foundopt ? foundopt->shopt : L'?';
}
