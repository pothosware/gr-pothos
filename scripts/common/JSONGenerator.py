#
# Copyright 2020 Free Software Foundation, Inc.
#           2020 Nicholas Corgan
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

from gnuradio.blocktool import BlockHeaderParser, GenericHeaderParser

import json
import os

class JSONGenerator:

    def __init__(self, prefix, namespace, prefix_include_root, output_dir, addl_includes):
        """Initialize JSONGenerator
        prefix -- path to installed gnuradio prefix (use gr.prefix() if unsure)
        namespace -- desired namespace to parse e.g. ['gr','module_name']
            module_name is stored as the last element of namespace
        prefix_include_root -- relative path to module headers, e.g. "gnuradio/modulename"

        Keyword arguments:
        output_dir -- path where bindings will be placed
        addl_includes -- comma separated list of additional include directories (default "")
        match_include_structure --
            If set to False, a bindings/ dir will be placed directly under the specified output_dir
            If set to True, the directory structure under include/ will be mirrored
        """

        self.header_extensions = ['.h', '.hh', '.hpp']
        self.addl_include = addl_includes
        self.prefix = prefix
        self.namespace = namespace
        self.module_name = namespace[-1]
        self.prefix_include_root = prefix_include_root
        self.output_dir = output_dir

        pass

    def generateSingleJSON(self, file_to_process):
        """Produce the blockname_python.cc python bindings"""
        output_dir = self.getOutputDir(file_to_process)
        binding_pathname = None
        base_name = os.path.splitext(os.path.basename(file_to_process))[0]
        module_include_path = os.path.abspath(os.path.dirname(file_to_process))
        top_include_path = os.path.join(
            module_include_path.split('include'+os.path.sep)[0], 'include')

        include_paths = ','.join(
            (module_include_path, top_include_path))
        if self.prefix:
            prefix_include_path = os.path.abspath(
                os.path.join(self.prefix, 'include'))
            include_paths = ','.join(
                (include_paths, prefix_include_path)
            )
        if self.addl_include:
            include_paths = ','.join((include_paths, self.addl_include))

        parser = GenericHeaderParser(
            include_paths=include_paths, file_path=file_to_process)
        try:
            header_info = parser.get_header_info(self.namespace)

            self.writeJSON(header_info, base_name, output_dir)

        except Exception as e:
            print(e)
            failure_pathname = os.path.join(
                output_dir, 'failed_conversions.txt')
            with open(failure_pathname, 'a+') as outfile:
                outfile.write(file_to_process)
                outfile.write(str(e))
                outfile.write('\n')

        return binding_pathname

    def getFileList(self, include_path):
        """Recursively get sorted list of files in path"""
        file_list = []
        for root, _, files in os.walk(include_path):
            for file in files:
                _, file_extension = os.path.splitext(file)
                if (file_extension in self.header_extensions):
                    pathname = os.path.abspath(os.path.join(root, file))
                    file_list.append(pathname)
        return sorted(file_list)

    def writeJSON(self, header_info, base_name, output_dir):
        json_pathname = os.path.join(output_dir, '{}.json'.format(base_name))
        with open(json_pathname, 'w') as outfile:
            json.dump(header_info, outfile)

    def getOutputDir(self, filename):
        """Get the output directory for a given file"""
        output_dir = self.output_dir
        if output_dir and not os.path.exists(output_dir):
            output_dir = os.path.abspath(output_dir)
            os.makedirs(output_dir)

        return output_dir

    def generateJSON(self, module_dir):
        """Generate JSON for the given GR module

        module_dir -- path to the include directory where the public headers live
        """
        file_list = self.getFileList(module_dir)
        api_pathnames = [s for s in file_list if 'api.h' in s]
        for f in api_pathnames:
            file_list.remove(f)
        for fn in file_list:
            self.generateSingleJSON(fn)
