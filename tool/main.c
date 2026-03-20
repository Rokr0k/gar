#include <gar.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <dirent.h>
#endif

static void print_version(const char *arg0);
static void print_help(const char *arg0, const char *cmd);

static void archive(int argc, char **argv);
static void archive_item(gar_writer_t *wr, const char *item);

static void extract(int argc, char **argv);

int main(int argc, char **argv) {
  if (argc < 2) {
    print_help(argv[0], NULL);
  } else if (strcmp(argv[1], "version") == 0) {
    print_version(argv[0]);
  } else if (strcmp(argv[1], "help") == 0) {
    if (argc < 3) {
      print_help(argv[0], NULL);
    } else {
      print_help(argv[0], argv[2]);
    }
  } else if (strcmp(argv[1], "archive") == 0) {
    archive(argc, argv);
  } else if (strcmp(argv[1], "extract") == 0) {
    extract(argc, argv);
  } else {
    print_help(argv[0], NULL);
  }

  return 0;
}

void print_version(const char *arg0) { printf("%s: %s\n", arg0, gar_version); }

void print_help(const char *arg0, const char *cmd) {
  if (cmd == NULL) {
    printf("Usage: %s [COMMAND] [...ARGUMENTS]\n", arg0);
    printf("\n");
    printf("Commands:\n");
    printf("  version   Show version information.\n");
    printf("  help      Show this message.\n");
    printf("  archive   Create a GAR file.\n");
    printf("  extract   Extract a file from a GAR file.\n");
    printf("\n");
    printf("Details about these commands can be found by running:\n");
    printf("  %s help [COMMAND]\n", arg0);
    printf("\n");
  } else if (strcmp(cmd, "version") == 0) {
    printf("Usage: %s version\n", arg0);
    printf("\n");
    printf("Like, do you want the version in different formats?\n");
    printf("I don't even know how to put git hash into executables.\n");
    printf("\n");
  } else if (strcmp(cmd, "help") == 0) {
    printf("Usage: %s help [COMMAND]\n", arg0);
    printf("\n");
    printf("You know what? This is actually comical.\n");
    printf("You needed help about getting help.\n");
    printf("\n");
  } else if (strcmp(cmd, "archive") == 0) {
    printf("Usage: %s archive [GAR_FILE] [...FILES|DIRS]\n", arg0);
    printf("\n");
    printf("Create an archive file that contains the following files and "
           "directories.\n");
    printf("Search is done recursively.\n");
    printf("\n");
  } else if (strcmp(cmd, "extract") == 0) {
    printf("Usage: %s extract [GAR_FILE] [...ITEMS]\n", arg0);
    printf("\n");
    printf("Extract a file from the archive file.\n");
    printf("All files in GAR are stored by a hash, and therefore doesn't "
           "know the name of each of the files.\n");
    printf("You have to specify every items to extract the whole contents of "
           "a GAR file.\n");
    printf("\n");
  } else {
    print_help(arg0, NULL);
  }
}

void archive(int argc, char **argv) {
  if (argc < 3) {
    print_help(argv[0], "archive");
    return;
  }

  gar_writer_t *wr = gar_writer_alloc();
  if (wr == NULL) {
    printf("Error: gar_writer_alloc()\n");
    return;
  }

  if (gar_writer_set_file(wr, argv[2]) != 0) {
    printf("Error: gar_writer_set_file(wr, \"%s\")\n", argv[2]);
    gar_writer_free(wr);
    return;
  }

  for (int i = 3; i < argc; i++) {
    archive_item(wr, argv[i]);
  }

  if (gar_writer_finish(wr) != 0) {
    printf("Error: gar_writer_finish(wr)\n");
    gar_writer_free(wr);
    return;
  }

  gar_writer_free(wr);
}

void archive_item(gar_writer_t *wr, const char *item) {
  struct stat st;
  if (stat(item, &st) != 0) {
    printf("Error: stat(\"%s\", &st)\n", item);
    return;
  }

  switch (st.st_mode & S_IFMT) {
  case S_IFREG:
    if (gar_writer_add(wr, item, item) != 0) {
      printf("Error: gar_writer_add(wr, \"%s\", \"%s\")\n", item, item);
      return;
    }
    break;

  case S_IFDIR: {

#ifdef _WIN32

    size_t len_base = strlen(item);
    char *name = malloc(len_base + 3);
    strncpy(name, item, len_base);
    name[len_base] = '/';
    name[len_base + 1] = '*';
    name[len_base + 2] = '\0';

    WIN32_FIND_DATAA ffd;
    HANDLE hFind = FindFirstFileA(name, &ffd);
    if (hFind == INVALID_HANDLE_VALUE) {
      printf("Error: FindFirstFileA(\"%s\", &FindFileData)\n", name);
      return;
    }

    do {
      if (strcmp(ffd.cFileName, ".") == 0 || strcmp(ffd.cFileName, "..") == 0) {
        continue;
      }

      size_t len_item = strlen(ffd.cFileName);
      char *n = malloc(len_base + len_item + 2);
      strncpy(n, name, len_base + 1);
      strncpy(n + len_base + 1, ffd.cFileName, len_item);
      n[len_base + len_item + 1] = '\0';

      archive_item(wr, n);

      free(n);
    } while (FindNextFileA(hFind, &ffd) != 0);

    FindClose(hFind);
    free(name);

#else

    DIR *dir = opendir(item);
    if (dir == NULL) {
      printf("Error: opendir(\"%s\")\n", item);
      return;
    }

    struct dirent *dirent;
    while ((dirent = readdir(dir)) != NULL) {
      if (strcmp(dirent->d_name, ".") == 0 ||
          strcmp(dirent->d_name, "..") == 0) {
        continue;
      }

      size_t len_base = strlen(item);
      size_t len_item = strlen(dirent->d_name);
      char *name = malloc(len_base + len_item + 2);
      strncpy(name, item, len_base);
      name[len_base] = '/';
      strncpy(name + len_base + 1, dirent->d_name, len_item);
      name[len_base + len_item + 1] = '\0';

      archive_item(wr, name);

      free(name);
    }

    closedir(dir);

#endif

    break;
  }
  }
}

void extract(int argc, char **argv) {
  if (argc < 3) {
    print_help(argv[0], "extract");
    return;
  }

  gar_reader_t *rd = gar_reader_alloc();
  if (rd == NULL) {
    printf("Error: gar_reader_alloc()\n");
    return;
  }

  if (gar_reader_open(rd, argv[2]) != 0) {
    printf("Error: gar_reader_open(rd, \"%s\")\n", argv[2]);
    gar_reader_free(rd);
    return;
  }

  for (int i = 3; i < argc; i++) {
    uint32_t id;
    if (gar_reader_find(rd, argv[i], &id) != 0) {
      printf("Error: gar_reader_find(rd, \"%s\", &id)\n", argv[i]);
      continue;
    }

    uint64_t size;
    if (gar_reader_size(rd, id, &size) != 0) {
      printf("Error: gar_reader_size(rd, \"%s\" as id, &size)\n", argv[i]);
      continue;
    }

    void *ptr = malloc(size);
    if (ptr == NULL) {
      printf("Error: buffer(%" PRIu64 ")\n", size);
      continue;
    }

    if (gar_reader_read(rd, id, ptr, &size) != 0) {
      printf("Error: gar_reader_read(rd, \"%s\" as id, ptr, &size)\n", argv[i]);
      free(ptr);
      continue;
    }

    FILE *fp = fopen(argv[i], "wb");
    if (fp == NULL) {
      printf("Error: fopen(\"%s\", \"wb\")\n", argv[i]);
      free(ptr);
      continue;
    }

    if (fwrite(ptr, 1, size, fp) < size) {
      printf("Error: fwrite(ptr, 1, size, fp) < size\n");
      free(ptr);
      continue;
    }

    fclose(fp);
    free(ptr);
  }

  gar_reader_free(rd);
}
