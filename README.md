# FAT32Reader
FAT32 image reader for EECS3540: Systems Programming

This is a FAT32 file system reader. Must be passed a .img file of a FAT32 system in the command line. 
Works when the .img file is in the same folder as the program. Passing a different path may work, but I've not tried it.

The following commands are implemented:
    DIR: displays all non-hidden files and directories in the current directory
    EXTRACT: extract a file in the current directory into the directory that the program is running in
    (EXTRA CREDIT) CD: Change directory to a directory within the current folder, or .. to go back
