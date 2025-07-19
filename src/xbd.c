#include "orta.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char *filename;
    long size;
    char *data;
} Bundle;

typedef struct {
    char *main_executable;
    int file_count;
    Bundle *bundles;
} BundleData;

typedef struct {
    char **filenames;
    int count;
    char *main_executable;
} UnbundledFiles;

static inline int removeFile(const char *filename) {
    return remove(filename);
}

static void freeBundleData(BundleData *bd) {
    if (!bd) return;

    free(bd->main_executable);

    if (bd->bundles) {
        for (int i = 0; i < bd->file_count; i++) {
            free(bd->bundles[i].filename);
            free(bd->bundles[i].data);
        }
        free(bd->bundles);
    }
}

static void freeUnbundledFiles(UnbundledFiles *uf) {
    if (!uf) return;

    if (uf->filenames) {
        for (int i = 0; i < uf->count; i++) {
            if (uf->filenames[i]) {
                removeFile(uf->filenames[i]);
                free(uf->filenames[i]);
            }
        }
        free(uf->filenames);
    }
    free(uf->main_executable);
}

static int loadFilesIntoBundles(char **filenames, int count, BundleData *bd) {
    bd->bundles = calloc(count, sizeof(Bundle));
    if (!bd->bundles) return -1;

    for (int i = 0; i < count; i++) {
        FILE *f = fopen(filenames[i], "rb");
        if (!f) goto cleanup;

        fseek(f, 0, SEEK_END);
        long size = ftell(f);
        rewind(f);

        bd->bundles[i].filename = strdup(filenames[i]);
        bd->bundles[i].size = size;
        bd->bundles[i].data = malloc(size);

        if (!bd->bundles[i].data || !bd->bundles[i].filename) {
            fclose(f);
            goto cleanup;
        }

        if (fread(bd->bundles[i].data, 1, size, f) != size) {
            fclose(f);
            goto cleanup;
        }
        fclose(f);
    }
    return 0;

cleanup:
    for (int j = 0; j < count; j++) {
        free(bd->bundles[j].filename);
        free(bd->bundles[j].data);
    }
    free(bd->bundles);
    bd->bundles = NULL;
    return -1;
}

int BundleFiles(char **filenames, int count, const char *bundlename, const char *runfile) {
    BundleData bd = {0};
    bd.file_count = count;
    bd.main_executable = strdup(runfile);

    if (!bd.main_executable || loadFilesIntoBundles(filenames, count, &bd) != 0) {
        freeBundleData(&bd);
        return -1;
    }

    FILE *bundle = fopen(bundlename, "wb");
    if (!bundle) {
        freeBundleData(&bd);
        return -1;
    }

    int runfilelen = strlen(runfile);
    fwrite(&count, sizeof(int), 1, bundle);
    fwrite(&runfilelen, sizeof(int), 1, bundle);
    fwrite(runfile, 1, runfilelen, bundle);

    for (int i = 0; i < count; i++) {
        int namelen = strlen(bd.bundles[i].filename);
        fwrite(&namelen, sizeof(int), 1, bundle);
        fwrite(bd.bundles[i].filename, 1, namelen, bundle);
        fwrite(&bd.bundles[i].size, sizeof(long), 1, bundle);
        fwrite(bd.bundles[i].data, 1, bd.bundles[i].size, bundle);
    }

    fclose(bundle);
    freeBundleData(&bd);
    printf("Main executable: %s\n", runfile);
    return 0;
}

static char* readString(FILE *f, int len) {
    char *str = malloc(len + 1);
    if (!str || fread(str, 1, len, f) != len) {
        free(str);
        return NULL;
    }
    str[len] = '\0';
    return str;
}

UnbundledFiles* UnbundleFiles(const char *bundlename) {
    FILE *bundle = fopen(bundlename, "rb");
    if (!bundle) return NULL;

    UnbundledFiles *uf = calloc(1, sizeof(UnbundledFiles));
    if (!uf) {
        fclose(bundle);
        return NULL;
    }

    int runfilelen;
    if (fread(&uf->count, sizeof(int), 1, bundle) != 1 ||
        fread(&runfilelen, sizeof(int), 1, bundle) != 1) {
        fclose(bundle);
        free(uf);
        return NULL;
    }

    uf->main_executable = readString(bundle, runfilelen);
    if (!uf->main_executable) {
        fclose(bundle);
        free(uf);
        return NULL;
    }

    uf->filenames = calloc(uf->count, sizeof(char*));
    if (!uf->filenames) {
        fclose(bundle);
        freeUnbundledFiles(uf);
        free(uf);
        return NULL;
    }

    for (int i = 0; i < uf->count; i++) {
        int namelen;
        long size;

        if (fread(&namelen, sizeof(int), 1, bundle) != 1) goto error;

        uf->filenames[i] = readString(bundle, namelen);
        if (!uf->filenames[i]) goto error;

        if (fread(&size, sizeof(long), 1, bundle) != 1) goto error;

        char *data = malloc(size);
        if (!data || fread(data, 1, size, bundle) != size) {
            free(data);
            goto error;
        }

        FILE *outfile = fopen(uf->filenames[i], "wb");
        if (!outfile) {
            free(data);
            goto error;
        }

        fwrite(data, 1, size, outfile);
        fclose(outfile);
        free(data);
    }

    fclose(bundle);
    return uf;

error:
    fclose(bundle);
    freeUnbundledFiles(uf);
    free(uf);
    return NULL;
}

const char* short_to_flag(short flag) {
    switch (flag) {
        case FLAG_MEMORY:  return "MEMORY";
        case FLAG_STACK:   return "STACK";
        case FLAG_XCALL:   return "XCALL";
        default:           return "INVALID";
    }
}


int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage: %s bundle <files...> | %s run <bundlefile>\n", argv[0], argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "bundle") == 0) {
        if (argc < 5) {
            printf("Usage: %s bundle <files...> <main_executable>\n", argv[0]);
            return 1;
        }
        if (BundleFiles(&argv[2], argc - 3, "xbd.bundle", argv[argc-1]) == 0) {
            printf("Bundled %d files into xbd.bundle with main: %s\n", argc - 3, argv[argc-1]);
        } else {
            printf("Error bundling files\n");
            return 1;
        }
    } else if (strcmp(argv[1], "run") == 0) {
        UnbundledFiles *uf = UnbundleFiles(argv[2]);
        if (uf) {
            size_t len = strlen(uf->main_executable);
            if (len >= 5 && strcmp(uf->main_executable + len - 5, ".xbin") == 0) {
                OrtaVM vm = ortavm_create(uf->main_executable);
                load_xbin(&vm, uf->main_executable);
                execute_program(&vm);
                ortavm_free(&vm);
            }
            freeUnbundledFiles(uf);
            free(uf);
        } else {
            printf("Error unbundling files\n");
            return 1;
        }
    } else if (strcmp(argv[1], "list") == 0)
    {
        UnbundledFiles *uf = UnbundleFiles(argv[2]);
        if (uf) {
            printf("Main executable: %s\n", uf->main_executable);
            for (int i = 0; i < uf->count; i++) {
                printf("%d: %s\n", i, uf->filenames[i]);
            }
            freeUnbundledFiles(uf);
            free(uf);
        }
    } else if (strcmp(argv[1], "inspect") == 0) {
        UnbundledFiles *uf = UnbundleFiles(argv[2]);
        if (uf) {
            size_t len = strlen(uf->main_executable);
            if (len >= 5 && strcmp(uf->main_executable + len - 5, ".xbin") == 0) {
                OrtaVM vm = ortavm_create(uf->main_executable);
                load_xbin(&vm, uf->main_executable);
                printf("Found %d flags:\n", vm.meta.flags_count);
                for (int i = 0; i < vm.meta.flags_count; i++)
                {
                    printf("%s\n", short_to_flag(vm.meta.flags[i]));
                }
                ortavm_free(&vm);
            }
            freeUnbundledFiles(uf);
            free(uf);
        } else {
            printf("Error unbundling files\n");
            return 1;
        }
    } else {
        printf("Unknown command: %s\n", argv[1]);
        return 1;
    }

    return 0;
}