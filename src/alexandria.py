#!/usr/bin/env python

"""Alexandria: Quantum Package library.

Usage:
  alexandria add external  <name> <location>
  alexandria add immutable <name> <location> <children>...
  alexandria add mutable   <name> <location> <children>...
  alexandria type          <name>
  alexandria children      <name>
  alexandria remove        <name>
  alexandria installed     [--location] [--mutable]
"""
from docopt import docopt
import json
import irpy
import os
import sys

class alexandria(object):
    def __init__(self,arg):
        self.arg = arg

    @irpy.lazy_property
    def category(self):
        if self.arg['external']:
            return 1
        elif self.arg['immutable']:
            return 2
        elif self.arg['mutable']:
            return 3

    @irpy.lazy_property
    def qp_root(self):
        return os.environ["qp_root"]


    @irpy.lazy_property
    def json_file(self):
        directory = os.path.join(self.qp_root,"usr/share/json/")
        if not os.path.isdir(directory):
            os.mkdir(directory)
        return os.path.join(directory,"alexandria.json")

    @irpy.lazy_property
    def file_descriptor(self):
        if not os.path.isfile(self.json_file):
            return open(self.json_file, 'w+')
        if self.arg["add"] or self.arg["remove"]:
            return open(self.json_file,"r+")
        else:
            return open(self.json_file,"r")

    @irpy.lazy_property
    def data_json(self):
        try:
            return json.load(self.file_descriptor)
        except ValueError:
            return {}

    def save_db(self,data):
        self.file_descriptor.seek(0)
        json.dump(data,
                  self.file_descriptor,
                  sort_keys=True,
                  indent=4, separators=(',', ': '))

        self.file_descriptor.truncate()    

    def add(self):
        from collections import defaultdict
        name = self.arg["<name>"]

        d = {  "category": self.category,
               "location": self.arg["<location>"],
            }

        if self.category != 1:
            d["children"] = self.arg["<children>"]

        self.data_json[name] = d
        self.save_db(self.data_json)

    def remove(self):
        del self.data_json[self.arg["<name>"]]
        self.save_db(self.data_json)

    def type(self):
        print self.data_json[self.arg["<name>"]]["category"]

    def children(self):
        try:
            print "\n".join(self.data_json[self.arg["<name>"]]["children"])
        except KeyError:
            print ""

    def installed(self):

        data = self.data_json
        l_name = [name for name in data]

        if self.arg["--mutable"]:
            l_name = [name for name in l_name if data[name]["category"] == 3]
        
        if self.arg["--location"]:
            l_name = [data[name]["location"] for name in l_name]

        print "\n".join(l_name)

if __name__ == '__main__':

    arguments = docopt(__doc__)

    al = alexandria(arguments)
    if arguments["add"]:
        al.add()
    elif arguments["remove"]:
        al.remove()
    elif arguments["children"]:
        al.children()
    elif arguments["type"]:
        al.type()
    elif arguments["installed"]:
        al.installed()
