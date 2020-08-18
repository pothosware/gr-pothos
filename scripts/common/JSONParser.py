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

        self.jsonPaths = [os.path.join(self.moduleDir, p) for p in os.listdir(self.moduleDir) if p.endswith(".json")]
        for path in self.jsonPaths:
            self.includes += ["gnuradio/{0}/{1}.h".format(os.path.basename(os.path.dirname(path).replace("-","_")), os.path.splitext(os.path.basename(path))[0])]
            with open(os.path.abspath(path), "r") as f:
                self.allJSON += [json.load(f)]

    def getIncludes(self):
        return self.includes

    def getEnums(self):
        enums = []

        # BlockTool doesn't distinguish between enums and enum classes, so
        # we need to check the header to see if this is an enum class.
        #
        # See: https://github.com/gnuradio/gnuradio/issues/3703
        for fileJSON, include in zip(self.allJSON, self.includes):
            if len(fileJSON["namespace"]["enums"]) > 0:
                contents = ""
                with open(os.path.join(self.grInstallDir, "include", include),"r") as f:
                    contents = f.read()
                for enum in fileJSON["namespace"]["enums"]:
                    enumClassStr = "enum class {0}".format(enum["name"])
                    enum["isEnumClass"] = (enumClassStr in contents)
                    enums.append(enum)

        return enums

    def getClasses(self):
        classes = self.__getFieldForAllFiles("classes")
        # Filter out classes blocks not of our supported block types.
        SUPPORTED_BLOCK_TYPES = ["::gr::block", "::gr::sync_block", "::gr::sync_interpolator", "::gr::sync_decimator"]
        classes = [clazz for clazz in classes if "::".join(clazz[0].get("bases","")).replace("::::","::") in SUPPORTED_BLOCK_TYPES]

        # For each class, assemble the strings we'll need based on arguments, etc
        for clazz in classes:
            for func in clazz[0]["member_functions"]:
                if func["name"] == "make":
                    factoryArgs = []
                    makeCallArgs = []
                    for i,arg in enumerate(func["arguments"]):
                        # Hardcode some special cases
                        if ("itemsize" in arg["name"]) and ("size_t" in arg["dtype"]):
                            factoryArgs += ["const Pothos::DType& dtype"]
                            makeCallArgs += ["dtype.size()"]
                            func["dtype"] = "dtype"
                        elif ("sizeof_stream_item" in arg["name"]) and ("size_t" in arg["dtype"]):
                            factoryArgs += ["const Pothos::DType& dtype"]
                            makeCallArgs += ["dtype.size()"]
                            func["dtype"] = "dtype"
                        elif arg["dtype"].replace(" ","") == "char*":
                            factoryArgs += ["const std::string& {0}".format(arg["name"])]
                            makeCallArgs += ["const_cast<char*>({0}.c_str())".format(arg["name"])]
                        else:
                            factoryArgs += ["{0} {1}".format(arg["dtype"], arg["name"])]
                            makeCallArgs += [arg["name"]]

                    func["className"] = clazz[0]["name"]
                    func["factoryArgs"] = ", ".join(factoryArgs)
                    func["makeCallArgs"] = ", ".join(makeCallArgs)

                    # GNU Radio is inconsistent in its vlen names post-3.8, so check
                    # all of these
                    func["vlen"] = 1
                    for vlenName in ["vlen", "veclen", "vecLen"]:
                        vlenArgs = [arg for arg in func["arguments"] if arg["name"] == vlenName]
                        if vlenArgs:
                            func["vlen"] = vlenName
                            break

                    if "dtype" not in func: func["dtype"] = "Pothos::DType()"
                else:
                    continue

        return classes

    def getFactories(self):
        classes = self.getClasses()
        return [clazz[0]["member_functions"][0] for clazz in classes if clazz[0]["member_functions"][0]["name"] == "make"]

    def __getFieldForAllFiles(self, fieldName):
        outputs = []
        for fileJSON in self.allJSON:
            if len(fileJSON["namespace"][fieldName]) > 0:
                outputs.append(fileJSON["namespace"][fieldName])

        return outputs
