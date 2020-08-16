import argparse
from common import JSONParser
import mako.template
import os
import sys

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Generate *_wrapper.cc from JSON output')
    parser.add_argument('--prefix', help='Prefix of Installed GNU Radio')
    parser.add_argument('--input-dir', help='Input dir with all JSON')
    parser.add_argument('--output', help='Output file')
    args = parser.parse_args()

    parser = JSONParser.JSONParser(args.input_dir, args.prefix)

    namespace = os.path.basename(args.input_dir)
    enums = parser.getEnums()
    headers = parser.getIncludes()

    tmpl = None
    with open(os.path.join(os.path.dirname(__file__), "registration.tmpl.cpp"),"r") as f:
        tmpl = f.read()

    try:
        output = mako.template.Template(tmpl).render(namespace=namespace,enums=enums,headers=headers)
    except:
        print(mako.exceptions.text_error_template().render())
        sys.exit(1)

    with open(args.output,"w") as f:
        f.write(output)
