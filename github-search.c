#include <stdio.h>
#include <stdlib.h>
#include "parson.h"

#define next_argument() (argc--, argv++)
#define min(a, b) (a < b ? a : b);

struct _args {
    char *keyword;
    int limit;
};

int parse_args(int argc, char **argv, struct _args *args) {
    static const struct _args args_zero = { .limit = 1 };

    *args = args_zero;
    for (next_argument(); argc > 0; next_argument()) {
        if (argv[0][0] != '-') break;

	switch(argv[0][1]) {
	case 'n': {
	    next_argument();
	    if (argc == 0) {
	        printf("limit count required for -n option!\n");
		return 1;
	    }
	    args->limit = atoi(argv[0]);
	    break;
	}
	}
    }

    if (argc == 0) {
        printf("please specify keyword!\n\nusage:\n");
        return 1;
    }

    args->keyword = argv[0];
    
    return 0;
}

void usage(void) {
    printf("github-search [options] keyword\n");
    printf("-n  search count limit(default = 1).\n");
}

int main(int argc, char **argv) {
    char curl_command[512];
    char cleanup_command[256];
    char output_filename[] = "search_result.json";

    struct _args args;

    if (parse_args(argc, argv, &args) != 0) {
        usage();
	return 1;
    }
    
    sprintf(curl_command,
   	  "curl -s \"https://api.github.com/legacy/repos/search/%s\" > %s",
   	  args.keyword, output_filename);
    sprintf(cleanup_command, "rm -f %s", output_filename);
    system(curl_command);

    JSON_Value *root_value = json_parse_file(output_filename);
    if (json_value_get_type(root_value) != JSONObject) {
        json_value_free(root_value);
        system(cleanup_command);
        return 1;
    }

    JSON_Object *obj = json_value_get_object(root_value);
    JSON_Array *results = json_object_get_array(obj, "repositories");

    int count = min(json_array_get_count(results), args.limit);
    for (int i = 0; i < count; ++i) {
        JSON_Object *repo = json_array_get_object(results, i);
	const char *name = json_object_get_string(repo, "name");
	printf("https://github.com/%s/%s.git\n%s\n\n",
	       json_object_get_string(repo, "owner"),
	       name,
	       json_object_get_string(repo, "description")
	       );
    }
    
    json_value_free(root_value);
    system(cleanup_command);
    return 0;
}
