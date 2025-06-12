#!/usr/bin/env python3

"""
Create sets of DB C++ classes based on json schemas
"""

import os
import sys
import argparse
import shutil

# consts
IsPython3 = ( sys.version_info[0] >= 3 )

def cleanpath(pathtoclean):

    pathtoclean = os.path.abspath(pathtoclean)

    if os.path.exists(pathtoclean) and os.path.isdir(pathtoclean):
        shutil.rmtree(pathtoclean)

    if not os.path.exists(pathtoclean):
        os.makedirs(pathtoclean)

if __name__ == "__main__":

    usage = """
        CreateDBSourceFromJson

        CreateDBSourceFromJson [--schema|-s schema] [--outpath|-o outpath]
    """

    parser = argparse.ArgumentParser(usage=usage)

    parser.add_argument('--schema', '-s', 
                        help='schema file to process',
                        default='../src/database/config/DBTableSchema.json' 
                        )
    parser.add_argument('--outpath', '-o', 
                        help='folder path to store auto generated files',
                        default='../src/database/tables' 
                        )

    args = parser.parse_args()

    # try:

    schema = args.schema
    outpath = args.outpath
    cleanpath(outpath)
    os.system('python3 JsonToCpp.py -s ' + schema + ' -o ' + outpath)
    
    # except Exception as ex:
        # print("Exception : %s" %ex)

    # except KeyboardInterrupt:
        # print("Aborted by keyboard interrupt !")