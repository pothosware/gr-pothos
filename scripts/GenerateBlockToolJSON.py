import argparse
import os
from common import JSONGenerator

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Output JSON representation of a set of GNU Radio blocks')
    parser.add_argument('names', type=str, nargs='+',
                        help='Names of gr modules to bind (e.g. fft digital analog)')
    parser.add_argument('--output-dir', default='/tmp',
                        help='Output directory of generated bindings')
    parser.add_argument('--prefix', help='Prefix of Installed GNU Radio')
    parser.add_argument(
        '--include', help='Additional Include Dirs, comma separated', default='')
    args = parser.parse_args()

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
                                            output_dir, addl_includes=includes)
            jg.generateJSON(module_dir)

    # Write a dummy file that we'll use for the make target.
    make_target_path = os.path.join(output_dir, "make_target")
    with open(make_target_path,"w") as f:
        f.write("")
