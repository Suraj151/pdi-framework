#!/usr/bin/env python3

"""
Create sets of DB C++ classes based on json schemas
"""

import os
import sys

# consts
IsPython3 = ( sys.version_info[0] >= 3 )


if __name__ == "__main__":

    usage = """
        CreateDBSourceFromJson
    """

    try:

        os.system('python3 JsonToCpp.py -s ../src/database/config/DBTableSchema.json -o ../src/database/tables')
    
    except Exception as ex:
        print("Exception : %s" %ex)

    except KeyboardInterrupt:
        print("Aborted by keyboard interrupt !")