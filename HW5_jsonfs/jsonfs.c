#define FUSE_USE_VERSION 26

#include <stdio.h>
#include <stdlib.h>
#include <fuse.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <json-c/json.h>
#include <json-c/json_object.h>

int total_files = 0;
#define MAX_FILES 128
#define MAX_FILE_SIZE 4098
#define MAX_DIR_ENTRIES 16

typedef struct Entry {
    char *name;
    int inode;
    struct Entry *next;
} Entry;

typedef struct FileObject {
    int inode;
    char *type;
    char *name;
    char *data;
    Entry *entries;
    struct FileObject *next;
} FileObject;

Entry *new_entry(const char *name, int inode) {
    Entry *entry = malloc(sizeof(Entry));
    entry->name = strdup(name);
    entry->inode = inode;
    entry->next = NULL;
    return entry;
}

FileObject *head = NULL;
const char *filename;
const char *mount_point;

FileObject *new_file_object(const char *name, const char *type, const char *data) {
    FileObject *file = malloc(sizeof(FileObject));
    file->inode = total_files++;
    file->type = strdup(type);
    file->name = strdup(name);
    file->data = data ? strdup(data) : NULL;
    file->entries = NULL;
    file->next = NULL;
    return file;
}

void parse_json(struct json_object *json) {
    int n = json_object_array_length(json);
    json_object *val;

    for (int i = 0; i < n; i++) {
        struct json_object *obj = json_object_array_get_idx(json, i);

        FileObject *new_file_object = malloc(sizeof(FileObject));
        new_file_object->next = NULL;
        new_file_object->entries = NULL;
        new_file_object->data = NULL;

        if (json_object_object_get_ex(obj, "inode", &val)) {
            new_file_object->inode = json_object_get_int(val);
        }

        if (json_object_object_get_ex(obj, "type", &val)) {
            new_file_object->type = strdup(json_object_get_string(val));
        }

        if (json_object_object_get_ex(obj, "name", &val)) {
            new_file_object->name = strdup(json_object_get_string(val));
        }

        if (json_object_object_get_ex(obj, "data", &val)) {
            if (strcmp(new_file_object->type, "reg") == 0) {
                char *file_data = json_object_get_string(val);
                if (strlen(file_data) > MAX_FILE_SIZE) {
                    fprintf(stderr, "File exceeds maximum size\n");
                    exit(1);
                }
                new_file_object->data = strdup(file_data);
            }
        }

        if (json_object_object_get_ex(obj, "entries", &val)) {
            if (strcmp(new_file_object->type, "dir") == 0) {
                int num_entries = json_object_array_length(val);
                if (num_entries > MAX_DIR_ENTRIES) {
                    fprintf(stderr, "Directory has too many entries\n");
                    exit(1);
                }

                for (int j = 0; j < num_entries; j++) {
                    struct json_object *entry_obj = json_object_array_get_idx(val, j);
                    const char *name = json_object_get_string(json_object_object_get(entry_obj, "name"));
                    int inode = json_object_get_int(json_object_object_get(entry_obj, "inode"));
                    Entry *entry = new_entry(name, inode);

                    // Append the new entry to the end of the linked list
                    if (new_file_object->entries == NULL) {
                        new_file_object->entries = entry;
                    } else {
                        Entry *curr_entry = new_file_object->entries;
                        while (curr_entry->next != NULL) {
                            curr_entry = curr_entry->next;
                        }
                        curr_entry->next = entry;
                    }
                }
            }
        }

        if (total_files > MAX_FILES) {
            fprintf(stderr, "Exceeded maximum number of files\n");
            exit(1);
        }

        total_files++;

        printf("Created file object with inode %d, type %s\n",
               new_file_object->inode,
               new_file_object->type);

        if (new_file_object->entries != NULL) {
            printf("Entries:\n");
            Entry *curr_entry = new_file_object->entries;
            while (curr_entry != NULL) {
                printf("inode: %d, name: %s\n", curr_entry->inode, curr_entry->name);
                curr_entry = curr_entry->next;
            }
        }

        // add to linked list
        new_file_object->next = head;
        head = new_file_object;
    }
}

FileObject *find_file_by_path(const char *path) {
    FileObject *current = head;
    while (current != NULL) {
        if (strcmp(path, current->name) == 0) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

static int getattr_callback(const char *path, struct stat *stbuf) {
    printf("Calling getattr on path: %s\n", path);

    memset(stbuf, 0, sizeof(struct stat));

    path++;

    FileObject *current = head;
    while (current != NULL) {
        if (strcmp(path, current->name) == 0) {
            if (strcmp(current->type, "reg") == 0) {
                stbuf->st_mode = S_IFREG | 0777;
                stbuf->st_nlink = 1;
                stbuf->st_size = strlen(current->data);
            } else if (strcmp(current->type, "dir") == 0) {
                stbuf->st_mode = S_IFDIR | 0755;
                stbuf->st_nlink = 2;
            }
            return 0;
        }
        current = current->next;
    }

    return -ENOENT;
}

static int read_callback(const char *path, char *buf, size_t size, off_t offset,
                         struct fuse_file_info *fi) {
    path++;
    FileObject *file = find_file_by_path(path);
    if (file == NULL) {
        return -ENOENT;
    }
    if (strcmp(file->type, "dir") == 0) {
        return -EISDIR;
    }
    size_t len = strlen(file->data);
    if (offset < len) {
        if (offset + size > len)
            size = len - offset;
        memcpy(buf, file->data + offset, size);
    } else {
        size = 0;
    }
    return size;
}

static int readdir_callback(const char *path, void *buf, fuse_fill_dir_t filler,
                            off_t offset, struct fuse_file_info *fi) {
    path++;
    FileObject *current = head;
    while (current != NULL) {
        if (strcmp(path, current->name) == 0 && strcmp(current->type, "dir") == 0) {
            filler(buf, ".", NULL, 0);
            filler(buf, "..", NULL, 0);

            Entry *entry = current->entries;
            while (entry != NULL) {
                filler(buf, entry->name, NULL, 0);
                entry = entry->next;
            }

            return 0;
        }
        current = current->next;
    }

    return -ENOENT;
}

static int write_callback(const char *path, const char *buf, size_t size,
                          off_t offset, struct fuse_file_info *fi) {
    path++;

    FileObject *current = head;
    while (current != NULL) {
        if (strcmp(path, current->name) == 0) {
            if (strcmp(current->type, "dir") == 0) {
                return -EISDIR;
            }
            size_t len = strlen(current->data);
            if (offset + size > len) {
                // Expand the size of the data.
                current->data = realloc(current->data, offset + size + 1);
                // Set the new part to zero.
                memset(current->data + len, 0, offset + size + 1 - len);
            }
            // Write the new data.
            memcpy(current->data + offset, buf, size);
            return size;
        }
        current = current->next;
    }
    return -ENOENT;
}

static int unlink_callback(const char *path) {
    path++;
    FileObject *current = head;
    FileObject *prev = NULL;
    while (current != NULL) {
        if (strcmp(path, current->name) == 0) {
            // Unlink from the list.
            if (prev == NULL) {
                head = current->next;
            } else {
                prev->next = current->next;
            }
            // Free the file object.
            free(current->data);
            free(current);
            return 0;
        }
        prev = current;
        current = current->next;
    }
    return -ENOENT;
}

static int mkdir_callback(const char *path, mode_t mode) {
    path++;
    FileObject *current = head;
    while (current != NULL) {
        // Check if the directory already exists.
        if (strcmp(path, current->name) == 0) {
            return -EEXIST;
        }
        current = current->next;
    }
    // Create a new directory.
    FileObject *new_dir = new_file_object(path, "dir", NULL);
    new_dir->next = head;
    head = new_dir;
    return 0;
}

static int rmdir_callback(const char *path) {
    path++;
    FileObject *current = head;
    FileObject *prev = NULL;
    while (current != NULL) {
        if (strcmp(path, current->name) == 0) {
            if (strcmp(current->type, "dir") != 0) {
                return -ENOTDIR;
            }
            // Check if the directory is empty.
            if (current->entries != NULL) {
                return -ENOTEMPTY;
            }
            // Unlink from the list.
            if (prev == NULL) {
                head = current->next;
            } else {
                prev->next = current->next;
            }
            // Free the directory object.
            free(current);
            return 0;
        }
        prev = current;
        current = current->next;
    }
    return -ENOENT;
}

void dump_json(const char *filename) {
    struct json_object *json_array = json_object_new_array();

    FileObject *current = head;
    while (current != NULL) {
        json_object *json_obj = json_object_new_object();
        json_object_object_add(json_obj, "inode", json_object_new_int(current->inode));
        json_object_object_add(json_obj, "type", json_object_new_string(current->type));
        json_object_object_add(json_obj, "name", json_object_new_string(current->name));
        if (current->data != NULL)
            json_object_object_add(json_obj, "data", json_object_new_string(current->data));
        if (current->entries != NULL)
            json_object_object_add(json_obj, "entries", json_object_get(current->entries));
        json_object_array_add(json_array, json_obj);
        current = current->next;
    }

    if (json_object_to_file(filename, json_array) != 0) {
        fprintf(stderr, "Failed to save JSON file: %s\n", filename);
    }
    json_object_put(json_array);
}

Entry *find_entry_in_dir(FileObject *dir, const char *entry_name) {
    Entry *entry = dir->entries;
    while (entry != NULL) {
        if (strcmp(entry_name, entry->name) == 0) {
            return entry;
        }
        entry = entry->next;
    }
    return NULL;
}

static int open_callback(const char *path, struct fuse_file_info *fi) {
    path++;
    FileObject *file = find_file_by_path(path);
    if (file == NULL) {
        return -ENOENT;
    }
    if (strcmp(file->type, "dir") == 0) {
        return -EISDIR;
    }
    return 0;
}

static int create_callback(const char *path, mode_t mode, struct fuse_file_info *fi) {
    path++;
    if (find_file_by_path(path) != NULL) {
        return -EEXIST;
    }
    FileObject *file = new_file_object(path, "reg", "");
    file->next = head;
    head = file;
    return 0;
}

static int link_callback(const char *from, const char *to) {
    from++;
    to++;
    FileObject *file = find_file_by_path(from);
    if (file == NULL) {
        return -ENOENT;
    }
    if (find_file_by_path(to) != NULL) {
        return -EEXIST;
    }
    FileObject *link = new_file_object(to, file->type, file->data);
    link->entries = file->entries;
    link->next = head;
    head = link;
    return 0;
}

static int release_callback(const char *path, struct fuse_file_info *fi) {
    path++;
    return 0;
}

static int rename_callback(const char *from, const char *to) {
    from++;
    to++;
    FileObject *file = find_file_by_path(from);
    if (file == NULL) {
        return -ENOENT;
    }
    if (find_file_by_path(to) != NULL) {
        return -EEXIST;
    }
    free(file->name);
    file->name = strdup(to);
    return 0;
}

static struct fuse_operations jsonfs_oper = {
    .getattr = getattr_callback,
    .readdir = readdir_callback,
    .mkdir = mkdir_callback,
    .rmdir = rmdir_callback,
    .create = create_callback,
    .unlink = unlink_callback,
    .write = write_callback,
    .link = link_callback,
    .open = open_callback,
    .read = read_callback,
    .release = release_callback,
    .rename = rename_callback,
};

void print_json(struct json_object *json) {
    int n = json_object_array_length(json);

    for (int i = 0; i < n; i++) {
        struct json_object *obj = json_object_array_get_idx(json, i);

        printf("{\n");
        json_object_object_foreach(obj, key, val) {
            if (strcmp(key, "inode") == 0)
                printf("   inode: %d\n", (int)json_object_get_int(val));

            if (strcmp(key, "type") == 0)
                printf("   type: %s\n", (char *)json_object_get_string(val));

            if (strcmp(key, "name") == 0)
                printf("   name: %s\n", (char *)json_object_get_string(val));

            if (strcmp(key, "entries") == 0)
                printf("   # entries: %d\n", json_object_array_length(val));
        }
        printf("}\n");
    }
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <json input file> <mount point>\n", argv[0]);
        return 1;
    }

    filename = argv[1];
    mount_point = argv[2];

    struct json_object *fs_json = json_object_from_file(filename);
    if (fs_json != NULL) {
        parse_json(fs_json);
        json_object_put(fs_json);
    }

    argv[1] = argv[2];
    argv[2] = NULL;

    return fuse_main(argc - 1, argv, &jsonfs_oper, NULL);
}
