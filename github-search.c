#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include "parson.h"

#define next_argument() (argc--, argv++)
#define min(a, b) (a < b ? a : b);

struct _args {
    char *keyword;
    int limit;
};

struct _curl_data {
    char *buffer;
    size_t length;
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
                fprintf(stderr, "limit count required for -n option!\n");
                return 1;
            }
            args->limit = atoi(argv[0]);
            break;
        }
        }
    }

    if (argc == 0) {
        fprintf(stderr, "please specify keyword!\n\nusage:\n");
        return 1;
    }

    args->keyword = argv[0];
    
    return 0;
}

void usage(void) {
    printf("github-search [options] keyword\n");
    printf("-n  search count limit(default = 1).\n");
}

void free_curl_data(struct _curl_data *curl_data) {
    if (curl_data && curl_data->buffer) {
        free((void *)curl_data->buffer);
    }
    free((void *)curl_data);
}

size_t on_curl_data(char *ptr, size_t size, size_t nmemb, void *userdata) {
    struct _curl_data *curl_data = (struct _curl_data *) userdata;
    size_t data_size = size * nmemb;
    if (data_size == 0) {
        return 0;
    }
    if (!curl_data) {
        return data_size;
    }
    if (!curl_data->buffer) {
        curl_data->buffer = (char *)malloc(data_size);
    } else {
        curl_data->buffer = (char *)realloc(curl_data->buffer, curl_data->length + data_size);
    }
    if (curl_data->buffer) {
        memcpy(curl_data->buffer + curl_data->length, ptr, data_size);
        curl_data->length += data_size;
    }
    return data_size;
}

const char *load_json(const char *url) {
    struct _curl_data curl_data = { 0 };
    CURL *curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &curl_data);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, on_curl_data);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "github-search.c");
    curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    char *json = (char *) malloc(curl_data.length + 1);
    memcpy(json, curl_data.buffer, curl_data.length);
    json[curl_data.length] = '\0';
    
    return json;
}

int main(int argc, char **argv) {
    char endpoint[512];
    struct _args args;

    if (parse_args(argc, argv, &args) != 0) {
        usage();
        return 1;
    }

    sprintf(endpoint,
          "https://api.github.com/legacy/repos/search/%s",
          args.keyword);

    const char *json = load_json(endpoint);

    JSON_Value *root_value = json_parse_string(json);
    if (json_value_get_type(root_value) != JSONObject) {
        json_value_free(root_value);
        free((void*)json);
        return 1;
    }

    JSON_Object *obj = json_value_get_object(root_value);
    JSON_Array *results = json_object_get_array(obj, "repositories");

    if (!results) {
        fprintf(stderr, "error:\n %s\n", json);
    }
    
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
    free((void*)json);
    
    return 0;
}
