#
# Copyright 2020 Free Software Foundation, Inc.
#           2020 Nicholas Corgan
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
#

from .base import BindTool
from gnuradio.blocktool import BlockHeaderParser, GenericHeaderParser

import os
import pathlib
import json
from mako.template import Template
from datetime import datetime
import hashlib


class JSONGenerator:

    def __init__(self, prefix, namespace, prefix_include_root, output_dir="", addl_includes="",
                    match_include_structure=False, catch_exceptions=True, write_json_output=False, status_output=None,
                    flag_automatic=False, flag_pygccxml=False):
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
        self.match_include_structure = match_include_structure
        self.catch_exceptions = catch_exceptions
        self.write_json_output = write_json_output
        self.status_output = status_output
        self.flag_automatic = flag_automatic
        self.flag_pygccxml = flag_pygccxml

        pass

    def generateSingleJSON(self, file_to_process):
        """Produce the blockname_python.cc python bindings"""
        output_dir = self.get_output_dir(file_to_process)
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

            self.write_json(header_info, base_name, output_dir)

        except Exception as e:
            if not self.catch_exceptions:
                raise(e)
            print(e)
            failure_pathname = os.path.join(
                output_dir, 'failed_conversions.txt')
            with open(failure_pathname, 'a+') as outfile:
                outfile.write(file_to_process)
                outfile.write(str(e))
                outfile.write('\n')

        return binding_pathname

    def get_file_list(self, include_path):
        """Recursively get sorted list of files in path"""
        print("get_file_list: include_path="+include_path)
        file_list = []
        for root, _, files in os.walk(include_path):
            for file in files:
                _, file_extension = os.path.splitext(file)
                if (file_extension in self.header_extensions):
                    pathname = os.path.abspath(os.path.join(root, file))
                    file_list.append(pathname)
        return sorted(file_list)

    def write_json(self, header_info, base_name, output_dir):
        json_pathname = os.path.join(output_dir, '{}.json'.format(base_name))
        with open(json_pathname, 'w') as outfile:
            json.dump(header_info, outfile)

    def get_output_dir(self, filename):
        """Get the output directory for a given file"""
        output_dir = self.output_dir
        if output_dir and not os.path.exists(output_dir):
            output_dir = os.path.abspath(output_dir)
            print('creating output directory {}'.format(output_dir))
            os.makedirs(output_dir)

        return output_dir

    def generateJSON(self, module_dir):
        """Generate JSON for the given GR module

        module_dir -- path to the include directory where the public headers live
        """
        file_list = self.get_file_list(module_dir)
        print(file_list)
        api_pathnames = [s for s in file_list if 'api.h' in s]
        for f in api_pathnames:
            file_list.remove(f)
        for fn in file_list:
            self.generateSingleJSON(fn)
