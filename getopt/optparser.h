#pragma once

struct option {
	const char* name;
	bool has_arg;
	int* flag;
	int shopt;
};

#ifdef __cplusplus
class optparser {
private:
	int _argc;
	char** _argv;

	int _lastarg; // last found arg (for compaction)
	// we have to keep track of lastarg because we must defer compaction
	// until the end of all options in a single pack, but optind must stay
	// the same during pack processing.
	int _optind; // index of current option in argv
	int _optpos; // position of current option in argv[optind] (for packs)
	char* _optarg; // current argument (if any?)

	const option* _options;

public:
	void reset(int argc, char** argv, const option opts[]);
	int next();

	int get_index() const {
		return _optind;
	}

	char* get_arg() {
		return _optarg;
	}
};
#endif
