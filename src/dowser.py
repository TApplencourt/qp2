#!/usr/bin/env python
"""Dowser. Find the source.

Usage:
  dowser <source>

Returns the type of file and all the related information
"""
from docopt import docopt
import os

def user_real(source,tos):
    s = source.split("::")

    try:
        user, real = s[0],s[1]
    except IndexError:
        user = s[0] 
        real = user

    if tos=="url":
        user = os.path.basename(user)

    return user, real

def is_archive(full_path):
    ext = os.path.splitext(full_path)[1]

    if any([ext == e  for e in [".bz2",".gz",".xz",".tar","tbz2",".tgz",".zip"]]):
        return True
    else:
        return False

def directory_name(archive):
    directory_name, ext = os.path.splitext(archive)
    if ext in ['.gz', '.bz2']:
        directory_name = os.path.splitext(directory_name)[0]

    return directory_name

if __name__ == '__main__':
    arguments = docopt(__doc__)

    source=arguments["<source>"]

    tos="url" if "://" in source else "local"
    user_name, real_name = user_real(source,tos)

    result= [tos, real_name, user_name]

    if is_archive(user_name):
        directory_name = directory_name(user_name)
        result.extend(["directory",directory_name])
    
    print " ".join(result)
    
