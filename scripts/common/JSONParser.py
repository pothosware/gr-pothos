#
# Copyright 2020 Nicholas Corgan
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

import json
import os

class JSONParser:
    def __init__(self, moduleDir):
        allJSON = []
        includes = []

        if not os.path.exists(moduleDir):
            raise RuntimeError("The given module directory doesn't exist: "+moduleDir)

        jsonPaths = [p for p in os.listdir(moduleDir) if p.endswith(".json")]
        for path in jsonPaths:
            includes += ["gnuradio/{0}/{1}.hpp".format(os.path.basename(os.path.dirname(path)), os.path.splitext(os.path.basename(path))[0])]
            with open(os.path.abspath(path), "r") as f:
                allJSON += [json.load(f)]

    # Assumption: same for all files
    def getNamespace(self):
        return self.allJSON[0]["module_name"]

    def getIncludes(self):
        return self.includes

    def getEnums(self):
        return self.__getFieldForAllFiles("enums")

    def getClasses(self):
        return self.__getFieldForAllFiles("classes")

    def __getFieldForAllFiles(self, fieldName):
        outputs = []
        for fileJSON in self.allJSON:
            if len(fileJSON["namespace"][fieldName]) > 0:
                outputs.apend(fileJSON["namespace"][fieldName])

        return outputs
