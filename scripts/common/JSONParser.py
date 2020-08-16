#
# Copyright 2020 Nicholas Corgan
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

import json
import os

class JSONParser:
    def __init__(self, moduleDir, grInstallDir):
        self.moduleDir = moduleDir
        self.grInstallDir = grInstallDir
        self.allJSON = []
        self.includes = []

        if not os.path.exists(self.moduleDir):
            raise RuntimeError("The given module directory doesn't exist: "+self.moduleDir)

        self.jsonPaths = [p for p in os.listdir(self.moduleDir) if p.endswith(".json")]
        for path in self.jsonPaths:
            self.includes += ["gnuradio/{0}/{1}.h".format(os.path.basename(os.path.dirname(path)), os.path.splitext(os.path.basename(path))[0])]
            with open(os.path.abspath(path), "r") as f:
                self.allJSON += [json.load(f)]

    # Assumption: same for all files
    def getNamespace(self):
        return self.allJSON[0]["module_name"]

    # Account for the change from boost::shared_ptr to std::shared_ptr.
    # TODO: replace with single CMake check
    def getSPtrNamespace(self):
        namespaces = ["std", "boost"]

        for include in self.includes:
            with open(os.path.join(self.grInstallDir, include), "r") as f:
                contents = f.read()
                for namespace in namespaces:
                    if "typedef {0}::shared_ptr<".format(namespace) in contents:
                        return namespace

        # We should never get here, but oh well. Hope this works.
        return "std"

    def getIncludes(self):
        return self.includes

    def getEnums(self):
        return self.__getFieldForAllFiles("enums")

    def getClasses(self):
        classes = self.__getFieldForAllFiles("classes")

        # For each class, assemble the strings we'll need based on arguments, etc
        # TODO: replacements
        for clazz in classes:
            for func in clazz["member_functions"]:
                if func["name"] == "make":

                    factoryArgs = []
                    makeCallArgs = []
                    for i,arg in enumerate(func["arguments"]):
                        # Hardcode some special cases
                        if ("itemsize" in arg["name"]) and ("size_t" in arg["dtype"]):
                            factoryArgs += ["const Pothos::DType& itemsize"]
                            makeCallArgs += ["itemsize.size()"]
                        elif arg["dtype"] == "char*":
                            factoryArgs += ["const std::string& {0}".format(arg["name"])]
                            makeCallArgs += ["const_cast<char*>({0}.c_str())".format(arg["name"])]
                        else:
                            factoryArgs += ["{0} {1}".format(arg["dtype"], arg["name"])]
                            makeCallArgs += [arg["name"]]

                    func["factoryArgs"] = ", ".join(factoryArgs)
                    func["makeCallArgs"] = ", ".join(makeCallArgs)
                else:
                    continue

        return classes

    def __getFieldForAllFiles(self, fieldName):
        outputs = []
        for fileJSON in self.allJSON:
            if len(fileJSON["namespace"][fieldName]) > 0:
                outputs.apend(fileJSON["namespace"][fieldName])

        return outputs
