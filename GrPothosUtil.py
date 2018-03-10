########################################################################
## The GrPothosUtil helps to bind GR blocks into the Pothos plugin tree
########################################################################

########################################################################
## debug messages for build verbose
########################################################################
HEADER = '\033[95m'
OKBLUE = '\033[94m'
OKGREEN = '\033[92m'
WARNING = '\033[93m'
FAIL = '\033[91m'
ENDC = '\033[0m'

#try to use colorama to support ANSI color codes on windows
try:
    from colorama import init
    init()
except ImportError: pass

LOG = [""]

import sys
def header(msg, *args): sys.stderr.write(HEADER+msg%args+"\n"+ENDC);    LOG[0]+=msg%args+"\n"
def notice(msg, *args): sys.stderr.write(OKGREEN+msg%args+"\n"+ENDC);   LOG[0]+="I: "+msg%args+"\n"
def warning(msg, *args): sys.stderr.write(WARNING+msg%args+"\n"+ENDC);  LOG[0]+="W: "+msg%args+"\n"
def error(msg, *args): sys.stderr.write(FAIL+msg%args+"\n"+ENDC);       LOG[0]+="E: "+msg%args+"\n"
def blacklist(msg, *args): sys.stderr.write(OKBLUE+msg%args+"\n"+ENDC); LOG[0]+="B: "+msg%args+"\n"

class AttributeDict(dict):
    __getattr__ = dict.__getitem__
    __setattr__ = dict.__setitem__

########################################################################
## blacklists -- hopefully we can fix in the future
########################################################################
import json
import os

BLACKLIST_FILE = os.path.join(os.path.dirname(__file__), 'blacklist.json')
BLACKLIST_DATA = json.loads(open(BLACKLIST_FILE).read())

########################################################################
## directory and file utils
########################################################################
import fnmatch
import os

def glob_recurse(base, filt):
    for root, dirnames, filenames in os.walk(base):
      for filename in fnmatch.filter(filenames, filt):
          yield os.path.join(root, filename)

########################################################################
## single header inspection
########################################################################
import sys
import os
sys.path.append(os.path.dirname(__file__))
import CppHeaderParser

DISCOVERED_ENUMS = list()
ENUM_HEADERS = list()

KNOWN_BASES = [
    'block',
    'sync_block',
    'sync_interpolator',
    'sync_decimator',
]
def fix_KNOWN_BASES():
    for base in KNOWN_BASES:
        yield base
        yield 'gr::'+base
KNOWN_BASES = list(fix_KNOWN_BASES())

def tokenSplit(source):
    tok = ''
    for ch in source:
        if ch.isalnum() or ch == '_': tok += ch
        else:
            if tok: yield tok
            tok = ''
            yield ch
    if tok: yield tok

#the lexer can only handle C++ style enums
def reWriteEnums(source):
    out = list()
    typedefIndex = -1
    enumIndex = -1
    enumNameIndex = -1
    openBracketIndex = -1
    closeBracketIndex = -1
    for i, tok in enumerate(tokenSplit(source)):
        out.append(tok)
        if typedefIndex == -1 and tok == 'typedef': typedefIndex = i
        elif typedefIndex != -1 and enumIndex == -1 and tok == 'enum': enumIndex = i
        elif enumIndex != -1 and openBracketIndex == -1 and len(tok) > 1: enumNameIndex = i
        elif enumIndex != -1 and openBracketIndex == -1 and tok == '{': openBracketIndex = i
        elif openBracketIndex != -1 and closeBracketIndex == -1 and tok == '}': closeBracketIndex = i
        elif enumIndex != -1 and typedefIndex != -1 and openBracketIndex != -1 and closeBracketIndex != -1 and len(tok) > 1:
            if enumNameIndex != -1: out[enumNameIndex] = ''
            out[typedefIndex] = 'enum'
            out[enumIndex] = tok
            out[i] = ''
            typedefIndex = -1
            enumIndex = -1
            enumNameIndex = -1
            openBracketIndex = -1
            closeBracketIndex = -1
    return ''.join(out)

def is_this_class_a_block(className, classInfo):

    #check the blacklist before using
    fully_qualified = '::'.join([classInfo['namespace'], className])
    if fully_qualified in BLACKLIST_DATA:
        comment = BLACKLIST_DATA[fully_qualified]['comment']
        blacklist('Blacklisted class: %s - %s', fully_qualified, comment)
        return False

    for inherit in classInfo['inherits']:
        if inherit['access'] != 'public': return False
        if inherit['class'] in KNOWN_BASES: return True
    return False

def inspect_header(header_path):
    #notice('Inspecting: %s', header_path)
    contents = open(header_path).read()

    #remove API decl tokens so the lexer doesnt have to
    pp_tokens = list()
    for line in contents.splitlines():
        line = line.strip()
        if line.startswith('class '):
            api_decl = line.split()[1].strip()
            if api_decl.isupper(): pp_tokens.append(api_decl)
    for tok in set(pp_tokens): contents = contents.replace(tok, '')

    #rewrite typedef enums for the lexer into c++ style
    contents = reWriteEnums(contents)

    try: cppHeader = CppHeaderParser.CppHeader(contents, argType='string')
    except Exception as ex:
        warning('Inspect %s failed with %s', header_path, str(ex))
        return

    if cppHeader.enums:
        ENUM_HEADERS.append(header_path)
        DISCOVERED_ENUMS.extend(list(map(AttributeDict, cppHeader.enums)))

    return header_path, cppHeader

def gather_header_data(tree_paths, glob='*.h'):
    for tree_path in tree_paths:
        for header in glob_recurse(tree_path, glob):
            yield inspect_header(os.path.abspath(header))

def query_block_classes(cppHeader):
    for className, classInfo in cppHeader.CLASSES.items():
        if not is_this_class_a_block(className, classInfo): continue
        #notice('  Found: %s::%s', classInfo['namespace'], className)
        yield (className, classInfo)

########################################################################
## class info into a C++ source
########################################################################
REGISTRATION_TMPL_FILE = os.path.join(os.path.dirname(__file__), 'registration.tmpl.cpp')
from mako.template import Template

def classInfoIntoRegistration(**kwargs):
    tmpl_str = open(REGISTRATION_TMPL_FILE, 'r').read()
    return Template(tmpl_str).render(**kwargs)

########################################################################
## gather grc data
########################################################################
import xmltodict
import difflib
import copy

def gather_grc_data(tree_paths, glob='*.xml'):
    for tree_path in tree_paths:
        for xml_file in glob_recurse(tree_path, glob):
            yield os.path.splitext(os.path.basename(xml_file))[0], xmltodict.parse(open(xml_file).read())

def getGrcFileMatch(className, classInfo, grc_files):

    qualified_name = classInfo['namespace'].replace('::', '_')+'_'+className
    if qualified_name.startswith('gr_'): qualified_name = qualified_name[3:]

    #extract match?
    if qualified_name in grc_files: return qualified_name

    #make xxx* multi match
    def has_trailing_dtype(s): return '_' in s and s.split('_')[-1].isalnum() and len(s.split('_')[-1]) <= 3
    if has_trailing_dtype(qualified_name):
        for grc_file in grc_files: #ignore trailing end on compare
            if has_trailing_dtype(grc_file) and grc_file.split('_')[:-1] == qualified_name.split('_')[:-1]: return grc_file

        for grc_file in grc_files: #ignore trailing end on source
            if not has_trailing_dtype(grc_file) and grc_file.split('_') == qualified_name.split('_')[:-1]: return grc_file

    raise Exception('Cant find GRC match for %s'%className)

def grcCategoryRecurse(data, names=[]):
    if 'name' in data and data['name']:
        names.append(data['name'])

    if 'block' in data:
        blocks = data['block']
        if not isinstance(blocks, list): blocks = [blocks]
        for block in blocks: yield block, names

    if 'cat' in data:
        cats = data['cat']
        if not isinstance(cats, list): cats = [cats]
        for cat in cats:
            for x in grcCategoryRecurse(cat, copy.copy(names)): yield x

def grcBlockKeyToCategoryMap(grc_data):
    key_to_categories = dict()
    for file_name, data in grc_data.items():
        if 'cat' not in data: continue
        for blockKey, catNames in grcCategoryRecurse(data['cat']):
            catName = '/'.join(catNames)
            if blockKey not in key_to_categories:
                key_to_categories[blockKey] = list()
            key_to_categories[blockKey].append(catName)
    return key_to_categories

########################################################################
## doxygen to qt html
########################################################################
import cgi

def doxygenToDocLines(doxygen):
    in_unordered_list = False
    in_xmlonly = False

    for doxyline in doxygen.splitlines():
        doxyline = cgi.escape(doxyline)

        #supported html tags
        for tag in "A B BLOCKQUOTE BR CENTER CAPTION CODE DD DFN DIV DL DT EM HR H1 H2 H3 H4 H5 H6 I IMG LI OL P PRE SMALL SPAN STRONG SUB SUP TABLE TD TH TR TT KBD UL VAR".split():
            for tmpl in ['<%s>', '</%s>']:
                doxyline = doxyline.replace(cgi.escape(tmpl%tag.upper()), tmpl%tag.lower())

        #strip the front comment chars
        def front_strip(line, key):
            if line.startswith(key+' '): return line[len(key)+1:]
            if line.startswith(key): return line[len(key):]
            return line
        for begin in ('/*!', '*/', '//!', '//', '*'): doxyline = front_strip(doxyline, begin)
        if doxyline.strip().startswith('\\'): doxyline = doxyline.strip()
        for begin in ('\\brief', '\\details', '\\ingroup'): doxyline = front_strip(doxyline, begin)

        #htmlonly sections are ok -- keep
        if doxyline.startswith('\\htmlonly'): doxyline = doxyline.replace('\\htmlonly', '')
        if doxyline.startswith('\\endhtmlonly'): doxyline = doxyline.replace('\\endhtmlonly', '')

        #xmlonly sections are removed
        if doxyline.startswith('\\xmlonly'):
            in_xmlonly = True
            continue
        if in_xmlonly:
            if doxyline.startswith('\\endxmlonly'):
                in_xmlonly = False
            continue

        #unordered list support
        if doxyline.startswith('\\li'):
            if in_unordered_list:
                doxyline = doxyline.replace('\\li', '</li><li>')
            else:
                doxyline = doxyline.replace('\\li', '<ul><li>')
            in_unordered_list = True
        if in_unordered_list and not doxyline.strip():
            in_unordered_list = False
            yield '</li></ul>'

        #param - no special syntax
        if doxyline.startswith('\\param'): doxyline = doxyline.replace('\\param', '')

        #bold tags
        if doxyline.startswith('\\b'): doxyline = doxyline.replace('\\b', '<b>') + '</b>'

        #code blocks
        if doxyline.startswith('\\code'): doxyline = doxyline.replace('\\code', '<code>')
        if doxyline.startswith('\\endcode'): doxyline = doxyline.replace('\\endcode', '</code>')

        #formulas -- just do preformatted text for now
        if doxyline.startswith('\\f['): doxyline = doxyline.replace('\\f[', '<pre>')
        if doxyline.startswith('\\f]'): doxyline = doxyline.replace('\\f]', '</pre>')
        if doxyline.startswith('\\f{'): doxyline = doxyline.replace('\\f{', '<pre>')
        if doxyline.startswith('\\f}'): doxyline = doxyline.replace('\\f}', '</pre>')
        if '\\f$' in doxyline:
            doxyline = doxyline.replace('\\f$', '<pre>', 1)
            doxyline = doxyline.replace('\\f$', '</pre>', 1)

        #references -- put in italics
        if doxyline.startswith('\\sa'): doxyline = doxyline.replace('\\sa', '<i>') + '</i>'

        #sections -- become headings
        if doxyline.startswith('\\section'): doxyline = doxyline.replace('\\section', '<h2>') + '</h2>'
        if doxyline.startswith('\\subsection'): doxyline = doxyline.replace('\\subsection', '<h3>') + '</h3>'

        #single-word emphasis
        doxyline = re.sub("\\\\p(\\s*\\w+)", "<em>\\1</em>", doxyline)

        if doxyline.startswith('\\'): warning('doxyparse unknown field %s', doxyline)
        yield doxyline

########################################################################
## extract and process a single class
########################################################################
from collections import OrderedDict
import re

#was 8 for MSVC issues (_VARIADIC_MAX), which should be fixed in 2013 and up
MAX_ARGS = 32 #lets be reasonable...

def create_block_path(className, classInfo):
    ns = classInfo['namespace']
    ns = ns.replace('::', '/')
    if ns: return '/' + ns + '/' + className
    else: return '/' + className

def find_factory_function(className, classInfo, cppHeader):

    for method in classInfo['methods']['public']:
        if not method['static']: continue
        if 'make' not in method['name']: continue
        return method

    for function in cppHeader.functions:
        if 'make' not in function['name']: continue
        return function

    return None

KNOWN_FRAMEWORK_METHODS = [
    'general_work', 'work', 'forecast',
    'fixed_rate_noutput_to_ninput',
    'fixed_rate_ninput_to_noutput',
]

def find_block_methods(classInfo):
    for method in classInfo['methods']['public']:
        if method['static']: continue
        if method['constructor']: continue
        if method['destructor']: continue
        if method['name'] in KNOWN_FRAMEWORK_METHODS: continue
        if len(method['parameters']) > MAX_ARGS:
            warning("Too many parameters %s::%s ignored", classInfo['name'], method['name'])
            continue
        yield AttributeDict(method)

def parse_nested(text, left=r'[(]', right=r'[)]', sep=r','):
    """ Based on http://stackoverflow.com/a/17141899/190597 (falsetru) """
    pat = r'({}|{}|{})'.format(left, right, sep)
    tokens = re.split(pat, text)
    stack = [[]]
    for x in tokens:
        if not x or re.match(sep, x): continue
        if re.match(left, x):
            stack[-1].append([])
            stack.append(stack[-1][-1])
        elif re.match(right, x):
            stack.pop()
            if not stack:
                raise ValueError('error: opening bracket is missing')
        else:
            stack[-1].append(x)
    if len(stack) > 1:
        print(stack)
        raise ValueError('error: closing bracket is missing')
    return stack.pop()

def flatten_nested(nested, sep=r','):
    out = list()
    for n in nested:
        if isinstance(n, list) and out and not isinstance(out[-1], list):
            out[-1] += '(%s)'%(sep.join(flatten_nested(n)))
        else:
            out.append(n)
    return out

def match_function_param_to_key(function, argno, param, grc_make, grc_callbacks, grc_params, isFactory):

    def strip_off_nonalnum(s):
        out = ''
        for ch in s:
            if ch == '_' or ch.isalnum(): out += ch
            else: break
        return out

    #look through the make for a call to this function and inspect its args
    if isFactory:
        #strip the self.* method calls that show up on the lines after the factory
        grc_make = '\n'.join([l for l in grc_make.splitlines() if not l.startswith('self.')])

        #gather all of the function arguments that show up between parens
        make_match = [x for x in parse_nested(grc_make) if isinstance(x, list)]

        #take the longest set of arguments from the back
        make_args = list()
        for x in reversed(make_match):
            x = flatten_nested(x)
            if '#if' in str(x): continue #cant handle if/else/end in the args
            if len(x) > len(make_args): make_args = x

        #FIXME - block specific code is not ideal, can we do it in the blacklist?
        #some pfb blocks have pythonic wrappers that add attenuation
        #and its not actually in the C++ which is what we are looking at
        if 'pfb' in grc_make: make_args = [x for x in make_args if x.strip() != '$atten']

        #check the nth argument for a key match
        if len(make_args) > argno and '$' in str(make_args[argno]):
            make_arg = str(make_args[argno]).strip()
            make_arg = make_arg.split('$')[1]
            return strip_off_nonalnum(make_arg)

    #inspect callbacks for function matches to see if the param is the same
    if not isFactory:
        for callback in grc_callbacks:
            if function['name'] in callback:
                guts = callback.split(function['name'], 1)[1]
                args = list(map(strip_off_nonalnum, guts.split('$')))
                if args and not args[0]: args = args[1:]
                if len(args) == len(function['parameters']):
                    return args[argno]

    #lazy: get closest match or make a new param key
    matches = difflib.get_close_matches(param['name'], grc_params.keys(), n=1)
    if not matches: matches = [param['name']]
    return matches[0]

def get_as_list(data, key):
    try: out = data[key]
    except KeyError: out = list()
    if not isinstance(out, list): out = [out]
    return out

def evalToJSON(opt):
    #support native booleans
    if opt == "True": return True
    if opt == "False": return False
    #support hexadecimal integers
    try: int(opt, 16); return opt
    except: pass
    #support other JSON types
    try: return json.loads(opt)
    except: pass
    #support the default variable samp_rate
    if opt == 'samp_rate': return 'rate'
    #support enums w/ python namespace
    if re.match('^[_a-z0-9]+\\.[_A-Z0-9]+$', opt):
        return '"%s"'%opt.split('.')[-1]
    #strip pmt calls, we can handle native expressions
    m = re.match('pmt\.\w+\((.+)\)', opt)
    if m: return m.groups()[0]
    #just quote the value
    return '"%s"'%opt

def evalToJSONStr(opt):
    val = evalToJSON(opt)
    if isinstance(val, str): return val
    return json.dumps(val)

def fromGrcParam(grc_param):
    param_d = dict(key=grc_param['key'])
    param_type = grc_param['type']
    options = get_as_list(grc_param, 'option')
    opts = [get_as_list(o, 'opt') for o in options]

    #parse name
    try: param_d['name'] = grc_param['name'] or "" #handles None
    except KeyError: pass

    #parse default
    try: param_d['default'] = evalToJSON(grc_param['value'] or "") #handles None
    except KeyError: pass

    #parse hide
    if 'hide' in grc_param:
        hide = grc_param['hide']
        if hide == 'part': param_d['preview'] = 'valid'
        elif hide == 'all': param_d['preview'] = 'disable'
        elif hide == 'none': param_d['preview'] = 'enable'
        else: param_d['preview'] = 'valid' #best guess

    if param_type == 'string':
        param_d['widgetType'] = 'StringEntry'

    if param_type == 'file_open':
        param_d['widgetType'] = 'FileEntry'
        param_d['widgetKwargs'] = dict(mode='open')

    if param_type == 'file_save':
        param_d['widgetType'] = 'FileEntry'
        param_d['widgetKwargs'] = dict(mode='save')

    if param_type == 'int':

        #only use the spinbox when the default value is int-parsable
        #we dont use spinbox for hex values and default expressions
        if 'value' in grc_param:
            try:
                int(grc_param['value'])
                param_d['widgetType'] = 'SpinBox'
            except Exception as ValueError: pass

        if 'default' not in param_d: param_d['default'] = 0

    if param_type in ('string', 'file_open', 'file_save'):
        if 'default' not in param_d: param_d['default'] = ""
        #must add quotes for the string-based values if the quotes are not present
        if not re.match('^".*"$', param_d['default']): param_d['default'] = '"%s"'%param_d['default']

    #generic combo-box style entry
    if options:
        param_d['options'] = [dict(name=o['name'], value=evalToJSONStr(o['key'])) for o in options]
        param_d['widgetType'] = 'ComboBox'
        param_d['widgetKwargs'] = dict(editable=param_type != 'enum')

    #support dtype widget entry
    optsHaveSize = opts and len(opts[0])==1 and opts[0][0].startswith('size')
    if param_type == 'enum' and optsHaveSize and 'type' in grc_param['key']:
        del param_d['options']
        param_d['default'] = '"complex64"'
        param_d['preview'] = 'disable'
        param_d['widgetType'] = 'DTypeChooser'
        param_d['widgetKwargs'] = dict(float=1,cfloat=1,int=1,cint=1,uint=1,cuint=1)

    return param_d

def stripConstRef(t):
    return t.replace('&', '').replace('const', '').strip()

def isConstCharStar(typeStr):
    return typeStr.count('const') == 1 and typeStr.count('char') == 1 and typeStr.count('*') == 1

def splitFactoryParam(factory_param):
    typeStr = factory_param['type']
    argName = factory_param['name']
    argPass = argName

    #handle special case of const char * input by replacing with std::string
    if isConstCharStar(typeStr):
        typeStr = typeStr.replace('char', 'std::string').replace('*', '&')
        argPass = argName + ".c_str()"

    #return c++ type, name of the argument, and code to pass into the factory
    return typeStr, argName, argPass

def getBlockInfo(className, classInfo, cppHeader, blockData, key_to_categories):

    #extract GRC data as lists
    grc_make = blockData['make']
    grc_params = OrderedDict([(p['key'], p) for p in get_as_list(blockData, 'param')])
    grc_callbacks = get_as_list(blockData, 'callback')
    grc_callbacks_str = ', '.join(grc_callbacks)

    #extract the factory method
    raw_factory = find_factory_function(className, classInfo, cppHeader)
    if not raw_factory:
        raise Exception('Cant find factory function for %s'%className)
    if 'parent' in raw_factory: factory_path = [className, raw_factory['name']]
    else: factory_path = [raw_factory['namespace'], raw_factory['name']]

    #determine calls (methods that arent getters)
    raw_calls = list()
    for method in find_block_methods(classInfo):
        name = method['name']
        if not method['parameters']: continue #ignore getters

        #check the blacklist before using
        fully_qualified = '::'.join([classInfo['namespace'], className, name])
        if fully_qualified in BLACKLIST_DATA:
            comment = BLACKLIST_DATA[fully_qualified]['comment']
            blacklist('Blacklisted method: %s - %s', fully_qualified, comment)
            continue

        #cant bind calls that use the const char * instead of std::string
        if any(list(map(isConstCharStar, [p['type'] for p in method['parameters']]))):
            warning('method %s::%s with "const char *" param is not supported', className, name)
            continue

        if name not in grc_make and name not in grc_callbacks_str:
            notice("method %s::%s not used in GRC %s", className, name, blockData['key'])
        else: raw_calls.append(method)

    #map all factory and call params to a unique param key
    fcn_param_to_key = dict()
    param_key_to_type = dict()
    for function in [raw_factory] + raw_calls:
        for i, param in enumerate(function['parameters']):
            param_key = match_function_param_to_key(
                function, i, param, grc_make, grc_callbacks, grc_params, function==raw_factory)
            fcn_param_to_key[(function['name'], param['name'])] = param_key
            param_key_to_type[param_key] = param['type']
    all_param_keys = set(fcn_param_to_key.values())

    #format arguments for calling into the block's factory
    exported_factory_args = list()
    internal_factory_args = list()
    used_factory_parameters = list()
    for factory_param in raw_factory['parameters']:
        typeStr, argName, argPass = splitFactoryParam(factory_param)
        exported_factory_args.append('%s %s'%(typeStr, argName))
        internal_factory_args.append(argPass)
        used_factory_parameters.append(factory_param)

    #determine nports
    nports_calls = list()
    for sink in get_as_list(blockData, 'sink'):
        if 'nports' not in sink: continue
        nports = sink['nports'].replace('$', '')
        if nports in grc_params:
            all_param_keys.add(nports)
            nports_calls.append(dict(args=[nports], name="__setNumInputs", type='initializer'))
    for source in get_as_list(blockData, 'source'):
        if 'nports' not in source: continue
        nports = source['nports'].replace('$', '')
        if nports in grc_params:
            all_param_keys.add(nports)
            nports_calls.append(dict(args=[nports], name="__setNumOutputs", type='initializer'))

    #determine aliases
    #skip over simple names like 'in' and 'out'
    alias_calls = list()
    index = 0
    for sink in get_as_list(blockData, 'sink'):
        if 'name' not in sink: continue
        if 'type' in sink and sink['type'] == 'message': continue
        alias = sink['name']
        if alias != 'in': alias_calls.append(
            dict(args=['"%d"'%index, evalToJSON(alias)], name="__setInputAlias", type='initializer'))
        index += 1
    index = 0
    for source in get_as_list(blockData, 'source'):
        if 'name' not in source: continue
        if 'type' in source and source['type'] == 'message': continue
        alias = source['name']
        if alias != 'out': alias_calls.append(
            dict(args=['"%d"'%index, evalToJSON(alias)], name="__setOutputAlias", type='initializer'))
        index += 1

    #determine params
    #first get the ones seen in the grc params
    #then do the ones that were found otherwise
    params = list()
    for param_key in grc_params.keys():
        if param_key not in all_param_keys: continue
        params.append(fromGrcParam(grc_params[param_key]))
    for param_key in all_param_keys:
        if param_key in grc_params: continue
        params.append(dict(key=param_key))
    params_d = dict([(param_d['key'], param_d) for param_d in params])

    #adjust factory args to use dtype
    for param_d in params:
        if 'widgetType' in param_d and param_d['widgetType'] == 'DTypeChooser':
            for i, arg in enumerate(internal_factory_args):
                try: key = fcn_param_to_key[('make', arg)]
                except KeyError: continue
                if key == param_d['key']: internal_factory_args[i] = '%s.size()'%arg
            for i, arg in enumerate(exported_factory_args):
                arg = arg.split()[-1]
                try: key = fcn_param_to_key[('make', arg)]
                except KeyError: continue
                if key == param_d['key']: exported_factory_args[i] = 'const Pothos::DType &%s'%arg

    #factory
    args = list()
    for param in used_factory_parameters:
        args.append(fcn_param_to_key[(raw_factory['name'], param['name'])])
    if len(args) > MAX_ARGS:
        raise Exception('Too many factory parameters for %s'%className)

    #calls (setters/initializers)
    calls = list()
    for function in raw_calls:
        call_type = 'setter' if function['name'] in grc_callbacks_str else 'initializer'
        call_d = dict(name=function['name'], args=list(), type=call_type)
        for param in function['parameters']:
            call_d['args'].append(fcn_param_to_key[(function['name'], param['name'])])
        calls.append(call_d)

    #install parameter docs from methods with doxygen
    for function in classInfo['methods']['public']:
        if 'doxygen' not in function: continue
        for doxyline in function['doxygen'].splitlines():
            m = re.match('^.*\\\\param\\s+(\\w+)\\s+(.*)$', doxyline)
            if not m: continue
            name, desc = m.groups()
            try:
                key = fcn_param_to_key[(function['name'], name)]
                params_d[key]['desc'] = [desc]
            except KeyError:
                matches = difflib.get_close_matches(name, params_d.keys(), n=1)
                if matches: params_d[matches[0]]['desc'] = [desc]

    #category extraction
    categories = list()
    try: categories.append(blockData['category'])
    except KeyError: pass
    try: categories.extend(key_to_categories[blockData['key']])
    except KeyError: pass
    if not categories: warning("Not block categories found: %s", className)
    categories = [c if c.startswith('/') else ('/'+c) for c in categories]
    #remove leading [Core] category name
    categories = [c.replace('[Core]', 'GNURadio') for c in categories]

    factoryInfo = AttributeDict(
        namespace=classInfo['namespace'],
        className=className,
        used_factory_parameters=used_factory_parameters,
        factory_function_path='::'.join(factory_path),
        exported_factory_args=', '.join(exported_factory_args),
        internal_factory_args=', '.join(internal_factory_args),
        block_methods=list(find_block_methods(classInfo)),
        path=create_block_path(className, classInfo),
        name=className
    )

    blockDesc = dict(
        path=create_block_path(className, classInfo),
        keywords=[className, classInfo['namespace'], blockData['key']],
        name=blockData['name'],
        categories=categories,
        calls=calls+nports_calls+alias_calls, #calls list
        params=params, #parameters list
        args=args, #factory function args
        docs=list(doxygenToDocLines(classInfo['doxygen'])),
    )

    return factoryInfo, blockDesc

def createMetaBlockInfo(grc_data, grc_file, info):

    #create a new type parameter for the new block desc
    type_param = None
    for param in get_as_list(grc_data[grc_file]['block'], 'param'):
        if 'type' in param['key'].lower():
            type_param = param
            break #take the first match
    if not type_param: raise Exception('bad association -- '+grc_data[grc_file]['block']['key'])

    #make dict of <opt>fcn:type_suffix</opt> or use key if fcn not found
    fcn_type_key_to_name = dict()
    for option in get_as_list(type_param, 'option'):
        fcn_type_key_to_name[option['key']] = option['name']
        for opt in get_as_list(option, 'opt'):
            if opt.startswith('fcn:'): fcn_type_key_to_name[opt[4:]] = option['name']

    #create the param and fill in options
    param_d = dict(key=type_param['key'], name=type_param['name'], preview='disable', options=list())
    for factory, blockDesc in info:
        name = factory['name'].replace('_', ' ').title()
        matches = difflib.get_close_matches(factory['name'].split('_')[-1], fcn_type_key_to_name.keys(), n=1)
        if matches: name = fcn_type_key_to_name[matches[0]]
        param_d['options'].append(dict(name=name, value='"%s"'%factory['name']))

    #create new block desc
    metaBlockDesc = None
    for factory, blockDesc in info:
        if metaBlockDesc is None or len(metaBlockDesc['args']) < len(blockDesc['args']):
            metaBlockDesc = copy.deepcopy(blockDesc) #copy block desc the most factory args
    assert(metaBlockDesc)
    metaBlockDesc['args'].insert(0, param_d['key'])
    metaBlockDesc['params'].insert(0, param_d)
    assert('_' in metaBlockDesc['path'])
    metaBlockDesc['path'] = metaBlockDesc['path'].rsplit('_', 1)[0]

    #subfactories for meta factory
    sub_factories = list()
    namespace = ''
    for factory, blockDesc in info:
        namespace = factory['namespace']
        internal_factory_args = ['a%d.convert<%s>()'%(i, stripConstRef(p['type'])) for i, p in enumerate(factory['used_factory_parameters'])]
        sub_factories.append(AttributeDict(
            name=factory['name'], internal_factory_args=', '.join(internal_factory_args)))

    #create metafactory
    metaFactoryArgs = ['const std::string &%s'%param_d['key']]
    metaFactoryArgs += ['const Pothos::Object &a%d'%i for i in range(len(metaBlockDesc['args'])-1)]
    metaFactory = AttributeDict(
        type_key=param_d['key'],
        name=grc_file,
        path=metaBlockDesc['path'],
        exported_factory_args=', '.join(metaFactoryArgs),
        sub_factories=sub_factories,
        namespace=namespace,
    )

    return metaFactory, metaBlockDesc


########################################################################
## main
########################################################################
import sys
import json
import traceback
from optparse import OptionParser

def main():

    #parse the input arguments
    parser = OptionParser()
    parser.add_option("--out", dest="out_path", help="output file path or 'stdout'")
    parser.add_option("--target", help="associated cmake library target name")
    parser.add_option("--prefix", help="installation prefix for gnuradio")
    parser.add_option("--log", help="dump log messages to specified file")
    (options, args) = parser.parse_args()

    #check input
    if options.target is None: raise Exception('GrPothosUtil requires --target')
    if options.prefix is None: raise Exception('GrPothosUtil requires --prefix')
    out_path = options.out_path
    header("GrPothosUtil begin: prefix=%s, target=%s, out=%s", options.prefix, options.target, out_path)

    #check paths
    grc_path = os.path.join(options.prefix, 'share', 'gnuradio', 'grc', 'blocks')
    header_path = os.path.join(options.prefix, 'include', 'gnuradio', options.target)
    if not os.path.exists(grc_path):
        error("grc path does not exist: " + grc_path)
        exit(-1)
    if not os.path.exists(header_path):
        error("gnuradio includes path does not exist: " + header_path)
        exit(-1)

    #generator information
    headers = list()
    factories = list()
    meta_factories = list()
    registrations = list()
    blockDescs = list()

    #extract grc metadata
    grc_data = dict(gather_grc_data([grc_path], glob=options.target+"_*.xml"))
    key_to_categories = grcBlockKeyToCategoryMap(grc_data)

    #extract header data
    header_data = gather_header_data([header_path])

    #extract info for each block class
    grc_file_to_meta_group = dict()
    for headerPath, cppHeader in header_data:
        for className, classInfo in query_block_classes(cppHeader):
            try:
                file_name = getGrcFileMatch(className, classInfo, grc_data.keys())
                factory, blockDesc = getBlockInfo(className, classInfo, cppHeader, grc_data[file_name]['block'], key_to_categories)
                if file_name not in grc_file_to_meta_group: grc_file_to_meta_group[file_name] = list()
                grc_file_to_meta_group[file_name].append((factory, blockDesc))
                headers.append(headerPath) #include header on success
            except Exception as ex:
                warning('%s: %s', options.target, str(ex))
                #print traceback.format_exc()

    #determine meta-block grouping -- one file to many keys
    for grc_file, info in grc_file_to_meta_group.items():
        if len(info) > 1:
            try:
                metaFactory, metaBlockDesc = createMetaBlockInfo(grc_data, grc_file, info)
                blockDescs.append(metaBlockDesc)
                for factory, blockDesc in info:
                    factories.append(factory)
                meta_factories.append(metaFactory)
                registrations.append(metaFactory) #uses keys: name and path

            except Exception as ex:
                error(str(ex))
                #print traceback.format_exc()
        else:
            for factory, blockDesc in info:
                factories.append(factory)
                registrations.append(factory) #uses keys: name and path
                blockDescs.append(blockDesc)

    #summary of findings
    notice('%s: Total factories        %d', options.target, len(factories))
    notice('%s: Total meta-factories   %d', options.target, len(meta_factories))
    notice('%s: Total enumerations     %d', options.target, len(DISCOVERED_ENUMS))
    notice('%s: Total registrations    %d', options.target, len(registrations))

    #generate output source
    sort_by_name = lambda l: sorted(l, key=lambda e: e['name'])
    output = classInfoIntoRegistration(
        headers=sorted(set(headers+ENUM_HEADERS)),
        enums=sort_by_name(DISCOVERED_ENUMS),
        factories=sort_by_name(factories),
        meta_factories=sort_by_name(meta_factories),
        registrations=sort_by_name(registrations),
        blockDescs=dict([(desc['path'], json.dumps(desc)) for desc in blockDescs]),
    )

    #send output to file or stdout
    if out_path:
        if out_path == 'stdout': print(output)
        else: open(out_path, 'w').write(output)

    #debug dumps of json blocks
    '''
    for desc in blockDescs:
        name = desc['path'].replace('/', '_')[1:]
        open(name + '.json', 'w').write(json.dumps(desc, indent=4))
    '''

    #dump the log messages to file
    if options.log and LOG[0]:
        open(options.log, 'w').write(LOG[0])

if __name__ == '__main__': main()
