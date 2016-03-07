/*
    FUSE: Filesystem in Userspace
    Copyright (C) 2001-2005  Miklos Szeredi <miklos@szeredi.hu>

    This program can be distributed under the terms of the GNU GPL.
    See the file COPYING.
*/

#define _FILE_OFFSET_BITS 64
#define FUSE_USE_VERSION 26
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#include <set>
#include <string>
using std::string;
using std::set;
set<string> dirs;       /* global register of directories */
set<string> files;      /* global register of files */

static int strendswith(const char *str, const char *sfx)
{
  size_t sfx_len = strlen(sfx);
  size_t str_len = strlen(str);
  if (str_len < sfx_len) return 0;
  return (strncmp(str + (str_len - sfx_len), sfx, sfx_len) == 0);
};

static int nullfs_isdir(const char *path)
{
  set<string>::const_iterator pos = dirs.find(string(path));
  if (pos != dirs.end()) return 1;
  return (strendswith(path, "/") || strendswith(path, "/..")
          || strendswith(path, "/.") || (strcmp(path, "..") == 0)
          || (strcmp(path, ".") == 0));
};

static void nullfs_getPath(string &uri, string &path)
{
  size_t found = uri.find_last_of("/");

  if (found != 0) {
    path = uri.substr(0, found);
  } else {
    path = "/";
  }
};

static void nullfs_getName(string &uri, string &name)
{
  size_t found = uri.find_last_of("/");

  name = uri.substr(found + 1);
};

static int nullfs_isfile(const char *path)
{
  set<string>::const_iterator pos = files.find(string(path));
  if ( (pos != files.end()) ) {
    return 1;
  } else {

#ifdef CREATE_AS_FILE_IF_NOT_EXIST
    files.insert(string(path));
    return 1;
#else
    return 0;
#endif
  }

};

static int nullfs_getattr(const char *path, struct stat *stbuf)
{
  int res = 0;

  memset(stbuf, 0, sizeof(struct stat));
  if (nullfs_isdir(path)) {
    stbuf->st_mode = S_IFDIR | 0777;
    stbuf->st_nlink = 3;
    stbuf->st_size = 0;
  } else if (nullfs_isfile(path)) {
    stbuf->st_mode = S_IFREG | 0666;
    stbuf->st_nlink = 1;
    stbuf->st_size = 0;
  } else {
    res = -ENOENT;
  };

  return res;
};

static int nullfs_readdir(const char *path, void *buf, fuse_fill_dir_t
                          filler, off_t offset, struct fuse_file_info *fi)
{
  (void) offset;
  (void) fi;
  int path_len = strlen(path);


  filler(buf, ".", NULL, 0);
  filler(buf, "..", NULL, 0);


  std::set<string>::iterator it;
  for (it = files.begin(); it != files.end(); ++it) {
    string curi = *it;
    string cpath;

    nullfs_getPath(curi, cpath);

    if (!memcmp(cpath.c_str(), path, cpath.length()) && cpath.length() == path_len)  {
      string name;
      nullfs_getName(curi, name);
      filler(buf, name.c_str(), NULL, 0);
    }
  }

  for (it = dirs.begin(); it != dirs.end(); ++it) {
    string curi = *it;
    string cpath;

    nullfs_getPath(curi, cpath);
    if (cpath == path && cpath.length() == path_len)  {
      string curi = *it;
      string name;
      nullfs_getName(curi, name);
      filler(buf, name.c_str(), NULL, 0);
    }
  }

  return 0;
};

static int nullfs_open(const char *path, struct fuse_file_info *fi)
{
  (void) fi;

  if (! nullfs_isfile(path)) return -ENOENT;

  return 0;
};

static int nullfs_read(const char *path, char *buf, size_t size,
                       off_t offset, struct fuse_file_info *fi)
{
  (void) buf;
  (void) size;
  (void) offset;
  (void) fi;

  if (! nullfs_isfile(path)) return -ENOENT;

  return 0;
};

static int nullfs_write(const char *path, const char *buf, size_t size,
                        off_t offset, struct fuse_file_info *fi)
{
  (void) buf;
  (void) offset;
  (void) fi;

  if (! nullfs_isfile(path)) return -ENOENT;

  return (int) size;
};

static int nullfs_mkdir(const char *path, mode_t m)
{
  (void) m;

  dirs.insert(string(path));

  return 0;
};

static int nullfs_create(const char *path, mode_t m,
                         struct fuse_file_info *fi)
{
  (void) m;
  (void) fi;

  files.insert(string(path));

  return 0;
};

static int nullfs_mknod(const char *path, mode_t m, dev_t d)
{
  (void) m;
  (void) d;

  files.insert(string(path));

  return 0;
};

static int nullfs_unlink(const char *path)
{
  (void) path;

  files.erase(string(path));

  return 0;
};

static int nullfs_rename(const char *src, const char *dst)
{
  (void) src;
  (void) dst;

  if (nullfs_isdir(src)) {
    dirs.erase(string(src));
    dirs.insert(string(dst));
  } else if (nullfs_isfile(src)) {
    files.erase(string(src));
    files.insert(string(dst));
  } else {
    return -ENOENT;
  };

  return 0;
};

static int nullfs_truncate(const char *path, off_t o)
{
  (void) path;
  (void) o;

  return 0;
};

static int nullfs_chmod(const char *path, mode_t m)
{
  (void) path;
  (void) m;

  return 0;
};

static int nullfs_chown(const char *path, uid_t u, gid_t g)
{
  (void) path;
  (void) u;
  (void) g;

  return 0;
};

static int nullfs_utimens(const char *path, const struct timespec ts[2])
{
  (void) path;
  (void) ts;

  return 0;
};


static int nullfs_statfs(const char *path, struct statvfs *stbuf)
{
  int res = 0;

  memset(stbuf, 0, sizeof(struct statvfs));

  // These will give a size of about 480 TB
  stbuf->f_bsize = 4096 * 1024;
  stbuf->f_blocks = (120 * 1024 * 1024);
  stbuf->f_bfree = (120 * 1024 * 1024);
  stbuf->f_bavail = (120 * 1024 * 1024);


  stbuf->f_files = 1;
  stbuf->f_ffree = 1;
  stbuf->f_fsid = 99999;

  return res;
};


static struct fuse_operations nullfs_oper;

int main(int argc, char *argv[])
{
  nullfs_oper.getattr = nullfs_getattr;
  nullfs_oper.readdir = nullfs_readdir;
  nullfs_oper.open = nullfs_open;
  nullfs_oper.read = nullfs_read;
  nullfs_oper.write = nullfs_write;
  nullfs_oper.create = nullfs_create;
  nullfs_oper.mknod = nullfs_mknod;
  nullfs_oper.mkdir = nullfs_mkdir;
  nullfs_oper.unlink = nullfs_unlink;
  nullfs_oper.rmdir = nullfs_unlink;
  nullfs_oper.truncate = nullfs_truncate;
  nullfs_oper.rename = nullfs_rename;
  nullfs_oper.chmod = nullfs_chmod;
  nullfs_oper.utimens = nullfs_utimens;
  nullfs_oper.statfs = nullfs_statfs;
  return fuse_main(argc, argv, &nullfs_oper, NULL);
};

/* vi:set sw=4 et tw=72: */
