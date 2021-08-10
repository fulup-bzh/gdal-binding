/*
 * Copyright (C) 2016-2018 "IoT.bzh"
 * Author Fulup Ar Foll <fulup@iot.bzh>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>
#include <errno.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h> 
#include <unistd.h>

#include "misc-utils.h"

// Fulup TDB waiting for afb integration
#ifndef AFB_ERROR
#define AFB_ERROR printf
#endif

#ifndef CONTROL_MAXPATH_LEN
#define CONTROL_MAXPATH_LEN 512
#endif

static int FdRemoveDir(int dirFd, const char *dirName)
{
    struct dirent* dirEnt;
    DIR *dirHandle;

    // open directory and keep track for its FD for openat function
    dirHandle= fdopendir(dirFd);
    if (!dirHandle) {
        AFB_ERROR ("Fail to open dir=%s error=%s", dirName, strerror(errno));
        goto OnErrorExit;
    }

    //AFB_NOTICE ("CONFIG-SCANNING:ctl_listconfig scanning: %s", dirPath);
    while ((dirEnt = readdir(dirHandle)) != NULL) {
        int err;

        // recursively search embedded directories ignoring any directory starting by '.'
        switch (dirEnt->d_type) {
            case DT_DIR: {
                if (dirEnt->d_name[0] == '.' ) continue;

                int newFd= openat (dirFd, dirEnt->d_name, O_RDONLY);
                if (newFd < 0) {
                    AFB_ERROR("Fail to read dir dir=%s entry=%s error=%s", dirName, dirEnt->d_name, strerror(errno)); 
                    goto OnErrorExit;
                }
                err = FdRemoveDir(newFd, dirEnt->d_name);
                if (err) goto OnErrorExit;

                err= unlinkat (dirFd, dirEnt->d_name, AT_REMOVEDIR);
                if (err) {
                    AFB_ERROR("Fail to unlink dir dir=%s error=%s", dirName, strerror(errno)); 
                    goto OnErrorExit;
                }

                break;
            }

            case DT_REG: 
            case DT_UNKNOWN: 
            case DT_LNK: 
                err= unlinkat (dirFd, dirEnt->d_name, 0);
                if (err) {
                    AFB_ERROR("Fail to unlink file dir=%s file=%s error=%s", dirName, dirEnt->d_name, strerror(errno)); 
                    goto OnErrorExit;
                }
                break;

            default: 
                AFB_ERROR("Fail to unlink file dir=%s file=%s unsupported file type", dirName, dirEnt->d_name);
                goto OnErrorExit;

        }
    }
    closedir(dirHandle);
    return 0;

OnErrorExit: 
    if (dirHandle) closedir(dirHandle);
    return -1;
}

int UtilsRemoveDir (const char* dirPath, removeDirE mode) {
    int dirFd, err;

    dirFd= open(dirPath, O_RDONLY);
    if (dirFd <0) {
        AFB_ERROR ("Fail to open dir=%s error=%s", dirPath, strerror(errno));
        goto NoDirExit;
    }

    err= FdRemoveDir(dirFd, dirPath);
    if (err < 0) goto OnErrorExit;

    close (dirFd);

    if (mode == RMDIR_PARENT_DEL) {
        int err= unlinkat(AT_FDCWD, dirPath, AT_REMOVEDIR);
        if (err < 0) {
            AFB_ERROR("Fail to unlink parent dir=%s error=%s", dirPath, strerror(errno));
            goto OnErrorExit;
        }
    }

    return 0;

NoDirExit: 
    return 1;

OnErrorExit: 
    return -1;
}
