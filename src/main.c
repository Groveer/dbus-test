#include "client.h"
#include "server.h"

#include <getopt.h>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void show_help(char *argv0)
{
    printf("Usage: %s -s | -c | -t <0/1>\n", argv0);
    printf("Options:\n");
    printf("  -h        Print this help message.\n");
    printf("  -s        Start for server mode.\n");
    printf("  -c        Start for client mode.\n");
    printf("  -t        Start for test mode.\n");
    printf("\n");
    printf("Examples:\n");
    printf("  linux>  %s -c -t 0\n", argv0);
}

typedef struct
{
    bool client;
    bool server;
    int test_type;
} Config;

int main(int argc, char *argv[])
{
    char opt;
    Config config = { false, false, 0 };
    while ((opt = getopt(argc, argv, "sct:")) != -1) {
        switch (opt) {
        case 's':
            config.server = true;
            break;
        case 'c':
            config.client = true;
            break;
        case 't': {
            const int tmp = atoi(optarg);
            if (tmp > 1 || tmp < 0) {
                show_help(argv[0]);
                exit(0);
            }
            config.test_type = tmp;
        } break;
        case '?':
            show_help(argv[0]);
            exit(0);
        }
    }
    int ret = 0;
    if (config.server) {
        ret = server_start();
    }
    if (config.client) {
        ret = client_start(config.test_type);
    }
    if (ret < 0) {
        printf("error: %s\n", strerror(-ret));
    }
    return 0;
}
