#!/usr/bin/python

import os;

mem_size = 8 * 1024; # 8K
diff = 8 * 1024;

while mem_size <= 16 * 1024 * 1024: # 16M
    cmd = "./bench " + str(mem_size) + " 0 1";
    os.system(cmd);
    if mem_size >= 32 * 1024:
        diff = 32 * 1024;
    elif mem_size >= 256 * 1024:
        diff = 256 * 1024;
    elif mem_size >= 8 * 1024 * 1024:
        diff = 1024 * 1024;
    mem_size = mem_size + diff;
