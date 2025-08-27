#!/usr/bin/env python3

"""
Create sets of C++ header config based on provided device
"""

import os
import sys
import re


# consts
IsPython3 = ( sys.version_info[0] >= 3 )


def build_classname(name, isAllUpperCase=False):

    if isAllUpperCase:
        return name.capitalize()
    else:
        return name[0].capitalize() + name[1:]

def clean_template(template, reformat=False):

    if reformat:
        template = re.sub(r'\n\s*\n', '\n\n', template)
    template = template.strip(os.linesep)
    template = template + os.linesep
    
    return template

def expand_token(template, token, tokenvalue, endl=False):

    template = template.replace(token, tokenvalue.strip(os.linesep))
    if endl:
        template = template + os.linesep
    return template

def format_file(formatter, filepath):

    if formatter == "clang":
        # Format the file with clang-formatter
        clangCmd = "clang-format --style=Microsoft -i " + filepath
        os.system(clangCmd)

def get_device_folder_relpath(device):
    
    devicepath = "../devices/" + device.lower()
    if os.path.isdir(devicepath):
        return devicepath
    else:
        print("Device folder path %s does not exist" % devicepath)
        return None

def get_device_id(device):
    
    return "DEVICE_" + device.upper()
