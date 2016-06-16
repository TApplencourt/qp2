#!/usr/bin/env python
"""Dowser. Find the source.

Usage:
  dowser <source>

Return the type of file and all the related information
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

def folder_name(archive):
    folder_name, ext = os.path.splitext(archive)
    if ext in ['.gz', '.bz2']:
        folder_name = os.path.splitext(folder_name)[0]

    return folder_name

if __name__ == '__main__':
    arguments = docopt(__doc__)

    source=arguments["<source>"]

    tos="url" if "://" in source else "local"
    user_name, real_name = user_real(source,tos)

    result= [tos, real_name, user_name]

    if is_archive(user_name):
        folder_name = folder_name(user_name)
        result.extend(["folder",folder_name])
    
    print " ".join(result)
    
