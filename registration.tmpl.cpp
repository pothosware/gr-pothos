//this is a machine generated file...

#include <Pothos/Framework.hpp>
#include <Pothos/Proxy.hpp>
#include <gnuradio/block.h>

using namespace gr;

/***********************************************************************
 * include block definitions
 **********************************************************************/
% for header in headers:
#include "${header}"
% endfor

/***********************************************************************
 * make GrPothosBlock wrapper with a gr::block
 **********************************************************************/
template <typename BlockType>
std::shared_ptr<Pothos::Block> makeGrPothosBlock(boost::shared_ptr<BlockType> block)
{
    auto block_ptr = boost::dynamic_pointer_cast<gr::block>(block);
    auto env = Pothos::ProxyEnvironment::make("managed");
    auto registry = env->findProxy("Pothos/BlockRegistry");
    return registry.call<std::shared_ptr<Pothos::Block>>("/gnuradio/block", block_ptr);
}

/***********************************************************************
 * create block factories
 **********************************************************************/
% for factory in factories:

% for ns in factory.namespace.split("::"):
namespace ${ns} {
% endfor

std::shared_ptr<Pothos::Block> factory__${factory.name}(${factory.exported_factory_args})
{
    auto __orig_block = ${factory.factory_function_path}(${factory.internal_factory_args});
    auto __pothos_block = makeGrPothosBlock(__orig_block);
    % if factory.block_methods:
    auto __orig_block_ref = std::ref(*static_cast<${factory.namespace}::${factory.className} *>(__orig_block.get()));
    % endif
    % for method in factory.block_methods:
    __pothos_block->registerCallable("${method.name}", Pothos::Callable(&${factory.namespace}::${factory.className}::${method.name}).bind(__orig_block_ref, 0));
    % endfor
    return __pothos_block;
}

% for ns in factory.namespace.split("::"):
} //namespace $ns
% endfor
% endfor

/***********************************************************************
 * meta block factories
 **********************************************************************/
% for factory in meta_factories:

% for ns in factory.namespace.split("::"):
namespace ${ns} {
% endfor

std::shared_ptr<Pothos::Block> factory__${factory.name}(${factory.exported_factory_args})
{
    % for sub_factory in factory.sub_factories:
    if (${factory.type_key} == "${sub_factory.name}") return factory__${sub_factory.name}(${sub_factory['internal_factory_args']});
    % endfor

    throw Pothos::RuntimeException("${factory.name} unknown type: "+${factory.type_key});
}

% for ns in factory.namespace.split("::"):
} //namespace $ns
% endfor
% endfor

/***********************************************************************
 * register block factories
 **********************************************************************/
% for registration in registrations:
static Pothos::BlockRegistry register__${registration.name}("${registration.path}", &${registration.namespace}::factory__${registration.name});
% endfor

/***********************************************************************
 * enum conversions
 **********************************************************************/
% for enum in enums:
${enum.namespace}${enum.name} int_to_${enum.name}(const int v)
{
    return ${enum.namespace}${enum.name}(v);
}
${enum.namespace}${enum.name} string_to_${enum.name}(const std::string &s)
{
    % for value in enum['values']:
    if (s == "${value['name']}") return ${enum.namespace}${value['name']};
    % endfor
    throw Pothos::RuntimeException("convert string to ${enum.name} unknown value: "+s);
}
% endfor

/***********************************************************************
 * register block descriptions and conversions
 **********************************************************************/
#include <Pothos/Plugin.hpp>

pothos_static_block(registerGrPothosUtilBlockDocs)
{
    % for path, blockDesc in blockDescs.items():
    <%
    escaped = ''.join([hex(ord(ch)).replace('0x', '\\x') for ch in blockDesc])
    %>
    Pothos::PluginRegistry::add("/blocks/docs${path}", std::string("${escaped}"));
    % endfor
    % for enum in enums:
    Pothos::PluginRegistry::add("/object/convert/gr_enums/int_to_${enum.name}", Pothos::Callable(&int_to_${enum.name}));
    Pothos::PluginRegistry::add("/object/convert/gr_enums/string_to_${enum.name}", Pothos::Callable(&string_to_${enum.name}));
    % endfor
}
