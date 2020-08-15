import argparse
import os
from core import JSONGenerator
import pathlib

parser = argparse.ArgumentParser(description='Bind a GR In-Tree Module')
parser.add_argument('names', type=str, nargs='+',
                    help='Names of gr modules to bind (e.g. fft digital analog)')

parser.add_argument('--output_dir', default='/tmp',
                    help='Output directory of generated bindings')
parser.add_argument('--prefix', help='Prefix of Installed GNU Radio')
parser.add_argument('--src', help='Directory of gnuradio source tree',
                    default=os.path.dirname(os.path.abspath(__file__))+'/../../..')
parser.add_argument(
    '--include', help='Additional Include Dirs, comma separated', default='')
args = parser.parse_args()

# NOTES:
# To generate QTGUI requires adding the QT headers as --include, e.g.
# bind_gr_module.py --prefix /share/gnuradio/grpybind --output_dir /share/tmp/take5
#   --include /usr/include/x86_64-linux-gnu/qt5/QtGui,/usr/include/x86_64-linux-gnu/qt5,
#   /usr/include/x86_64-linux-gnu/qt5/QtCore,/usr/include/x86_64-linux-gnu/qt5/QtWidgets,/usr/include/qwt qtgui

# To generate UHD requires adding the UHD headers, e.g.
# python3 /share/gnuradio/grpybind/src/gnuradio/gr-utils/python/bindtool/scripts/bind_gr_module.py
#   --prefix /share/gnuradio/grpybind
#   --include $UHD_PATH,$UHD_PATH/utils,$UHD_PATH/types,$UHD_PATH/transport,$UHD_PATH/usrp_clock,$UHD_PATH/rfnoc
#   --output_dir /share/tmp/take5 uhd


print(pathlib.Path(__file__).parent.absolute())
print(args)

prefix = args.prefix
output_dir = args.output_dir
includes = args.include
for name in args.names:
    namespace = ['gr', name.replace("-","_")]
    prefix_include_root = 'include/gnuradio/'+name
    module_dir = os.path.abspath(os.path.join(args.prefix, prefix_include_root))

    import warnings
    with warnings.catch_warnings():
        warnings.filterwarnings("ignore", category=DeprecationWarning)
        jg = JSONGenerator.JSONGenerator(prefix, namespace, prefix_include_root,
                                         output_dir, addl_includes=includes, match_include_structure=False, write_json_output=True)
        jg.generateJSON(module_dir)