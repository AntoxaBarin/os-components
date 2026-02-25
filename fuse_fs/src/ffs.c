/* FUSE File System */

#define FUSE_USE_VERSION 31
#include <errno.h>
#include <fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct inode inode_t;

typedef struct {
  char *name;
  inode_t *node;
} dir_entry_t;

struct inode {
  mode_t mode;
  size_t size;
  char *data;

  dir_entry_t *entries;
  size_t entry_count;

  inode_t *parent;
};

static inode_t *root;

static inode_t *inode_create(mode_t mode) {
  inode_t *n = calloc(1, sizeof(inode_t));
  n->mode = mode;
  return n;
}

static inode_t *dir_lookup(inode_t *dir, const char *name) {
  for (size_t i = 0; i < dir->entry_count; i++)
    if (!strcmp(dir->entries[i].name, name)) return dir->entries[i].node;
  return NULL;
}

static void dir_add(inode_t *dir, const char *name, inode_t *child) {
  dir->entries =
      realloc(dir->entries, sizeof(dir_entry_t) * (dir->entry_count + 1));

  dir->entries[dir->entry_count].name = strdup(name);
  dir->entries[dir->entry_count].node = child;

  dir->entry_count++;
  child->parent = dir;
}

static int dir_remove(inode_t *dir, const char *name) {
  for (size_t i = 0; i < dir->entry_count; i++) {
    if (!strcmp(dir->entries[i].name, name)) {
      free(dir->entries[i].name);
      memmove(&dir->entries[i], &dir->entries[i + 1],
              sizeof(dir_entry_t) * (dir->entry_count - i - 1));
      dir->entry_count--;
      return 0;
    }
  }
  return -ENOENT;
}

static inode_t *path_resolve(const char *path) {
  if (!strcmp(path, "/")) return root;

  char *p = strdup(path);
  char *token = strtok(p, "/");
  inode_t *cur = root;

  while (token && cur) {
    cur = dir_lookup(cur, token);
    token = strtok(NULL, "/");
  }

  free(p);
  return cur;
}

static inode_t *path_parent(const char *path, char **name) {
  char *dup = strdup(path);
  char *slash = strrchr(dup, '/');

  if (!slash) {
    free(dup);
    return NULL;
  }

  *name = strdup(slash + 1);

  if (slash == dup)
    slash[1] = 0;
  else
    *slash = 0;

  inode_t *p = path_resolve(dup);
  free(dup);
  return p;
}

static int ffs_getattr(const char *path, struct stat *st) {
  inode_t *n = path_resolve(path);
  if (!n) return -ENOENT;

  memset(st, 0, sizeof(struct stat));
  st->st_mode = n->mode;
  st->st_nlink = S_ISDIR(n->mode) ? 2 : 1;
  st->st_size = n->size;

  return 0;
}

static int ffs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                       off_t off, struct fuse_file_info *fi) {
  inode_t *dir = path_resolve(path);
  if (!dir || !S_ISDIR(dir->mode)) return -ENOENT;

  filler(buf, ".", NULL, 0);
  filler(buf, "..", NULL, 0);

  for (size_t i = 0; i < dir->entry_count; i++)
    filler(buf, dir->entries[i].name, NULL, 0);

  return 0;
}

static int ffs_mkdir(const char *path, mode_t mode) {
  char *name;
  inode_t *parent = path_parent(path, &name);
  if (!parent) return -ENOENT;

  inode_t *n = inode_create(S_IFDIR | mode);
  dir_add(parent, name, n);
  free(name);
  return 0;
}

static int ffs_mknod(const char *path, mode_t mode, dev_t dev) {
  char *name;
  inode_t *parent = path_parent(path, &name);
  if (!parent) return -ENOENT;

  inode_t *n = inode_create(S_IFREG | mode);
  dir_add(parent, name, n);
  free(name);
  return 0;
}

static int ffs_unlink(const char *path) {
  char *name;
  inode_t *parent = path_parent(path, &name);
  if (!parent) return -ENOENT;

  inode_t *n = dir_lookup(parent, name);
  if (!n) return -ENOENT;

  dir_remove(parent, name);

  free(n->data);
  free(n);
  free(name);
  return 0;
}

static int ffs_read(const char *path, char *buf, size_t size, off_t offset,
                    struct fuse_file_info *fi) {
  inode_t *n = path_resolve(path);
  if (!n) return -ENOENT;

  if (offset >= n->size) return 0;

  if (offset + size > n->size) size = n->size - offset;

  memcpy(buf, n->data + offset, size);
  return size;
}

static int ffs_write(const char *path, const char *buf, size_t size,
                     off_t offset, struct fuse_file_info *fi) {
  inode_t *n = path_resolve(path);
  if (!n) return -ENOENT;

  size_t newsize = offset + size;

  if (newsize > n->size) {
    n->data = realloc(n->data, newsize);
    n->size = newsize;
  }

  memcpy(n->data + offset, buf, size);
  return size;
}

static struct fuse_operations ffs_oper = {
    .getattr = ffs_getattr,
    .readdir = ffs_readdir,
    .mkdir = ffs_mkdir,
    .mknod = ffs_mknod,
    .unlink = ffs_unlink,
    .read = ffs_read,
    .write = ffs_write,
};

int main(int argc, char *argv[]) {
  root = inode_create(S_IFDIR | 0755);
  return fuse_main(argc, argv, &ffs_oper, NULL);
}