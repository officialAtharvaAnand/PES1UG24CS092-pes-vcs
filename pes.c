// pes.c — CLI entry point and command dispatch
//
// This file is PROVIDED. Do not modify.

#include "pes.h"
#include "index.h"
#include "commit.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

// ─── TODO: Command implementations ──────────────────────────────────────────

void cmd_init(void) {
    mkdir(PES_DIR, 0755);
    mkdir(OBJECTS_DIR, 0755);
    mkdir(".pes/refs", 0755);
    mkdir(REFS_DIR, 0755);
    // Create HEAD pointing to main branch
    FILE *f = fopen(HEAD_FILE, "w");
    if (f) { fprintf(f, "ref: refs/heads/main\n"); fclose(f); }
    printf("Initialized empty PES repository in %s/\n", PES_DIR);
}

void cmd_add(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: pes add <file>...\n");
        return;
    }
    Index *idx = calloc(1, sizeof(Index));
    if (!idx) { fprintf(stderr, "error: out of memory\n"); return; }
    if (index_load(idx) != 0) {
        fprintf(stderr, "error: failed to load index\n");
        free(idx); return;
    }
    for (int i = 2; i < argc; i++) {
        if (index_add(idx, argv[i]) != 0) {
            fprintf(stderr, "error: failed to add '%s'\n", argv[i]);
        }
    }
    free(idx);
}

void cmd_status(void) {
    Index *idx = calloc(1, sizeof(Index));
    if (!idx) { fprintf(stderr, "error: out of memory\n"); return; }
    if (index_load(idx) != 0) {
        idx->count = 0;
    }
    index_status(idx);
    free(idx);
}

void cmd_commit(int argc, char *argv[]) {
    const char *message = NULL;
    for (int i = 2; i < argc - 1; i++) {
        if (strcmp(argv[i], "-m") == 0) {
            message = argv[i + 1];
            break;
        }
    }
    if (!message) {
        fprintf(stderr, "error: commit requires a message (-m \"message\")\n");
        return;
    }
    ObjectID commit_id;
    if (commit_create(message, &commit_id) != 0) {
        fprintf(stderr, "error: commit failed\n");
        return;
    }
    char hex[HASH_HEX_SIZE + 1];
    hash_to_hex(&commit_id, hex);
    printf("Committed: %.12s... %s\n", hex, message);
}

static void log_callback(const ObjectID *id, const Commit *c, void *ctx) {
    (void)ctx;
    char hex[HASH_HEX_SIZE + 1];
    hash_to_hex(id, hex);
    printf("commit %s\n", hex);
    printf("Author: %s\n", c->author);
    printf("Date:   %llu\n\n", (unsigned long long)c->timestamp);
    printf("    %s\n\n", c->message);
}

void cmd_log(void) {
    if (commit_walk(log_callback, NULL) != 0) {
        printf("No commits yet.\n");
    }
}

// ─── PROVIDED: Command dispatch ─────────────────────────────────────────────

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: pes <command> [args]\n");
        fprintf(stderr, "\nCommands:\n");
        fprintf(stderr, "  init            Create a new PES repository\n");
        fprintf(stderr, "  add <file>...   Stage files for commit\n");
        fprintf(stderr, "  status          Show working directory status\n");
        fprintf(stderr, "  commit -m <msg> Create a commit from staged files\n");
        fprintf(stderr, "  log             Show commit history\n");
        return 1;
    }

    const char *cmd = argv[1];

    if      (strcmp(cmd, "init") == 0)     cmd_init();
    else if (strcmp(cmd, "add") == 0)      cmd_add(argc, argv);
    else if (strcmp(cmd, "status") == 0)   cmd_status();
    else if (strcmp(cmd, "commit") == 0)   cmd_commit(argc, argv);
    else if (strcmp(cmd, "log") == 0)      cmd_log();
    else {
        fprintf(stderr, "Unknown command: %s\n", cmd);
        fprintf(stderr, "Run 'pes' with no arguments for usage.\n");
        return 1;
    }

    return 0;
}
