//this is a machine generated file...

#include "GrPothosBlock/pothos_support.h"
#include <Pothos/Framework.hpp>
#include <Pothos/Proxy.hpp>
#include <gnuradio/block.h>

using namespace gr;

/***********************************************************************
 * include block definitions
 **********************************************************************/
% for header in headers:
#include <${header}>
% endfor

/***********************************************************************
 * make GrPothosBlock wrapper with a gr::block
 **********************************************************************/
template <typename BlockType>
static std::shared_ptr<Pothos::Block> makeGrPothosBlock(GRSPtr<BlockType> block, size_t vlen, const Pothos::DType& overrideDType)
{
    auto block_ptr = dynamicPointerCast<BlockType, gr::block>(block);
    auto env = Pothos::ProxyEnvironment::make("managed");
    auto registry = env->findProxy("Pothos/BlockRegistry");
    return registry.call<std::shared_ptr<Pothos::Block>>("/gnuradio/block", block_ptr, vlen, overrideDType);
}

/***********************************************************************
 * create block factories
 **********************************************************************/

// To disambiguate
using DeclareSampleDelayPtr = void(gr::block::*)(unsigned);

% for clazz in classes:
<%
    SKIP = ["squelch_base_cc", "squelch_base_ff"]
    if clazz[0]["name"] in SKIP: continue

    factory = [func for func in clazz[0]["member_functions"] if func["name"] == "make"][0]
    methods = [func for func in clazz[0]["member_functions"] if func["name"] != "make"]
%>

% for ns in namespace.split("::"):
namespace ${ns} {
% endfor

std::shared_ptr<Pothos::Block> factory__${clazz[0]["name"]}(${factory["factoryArgs"]})
{
    auto __orig_block = ${factory["className"]}::${factory["name"]}(${factory["makeCallArgs"]});
    auto __pothos_block = makeGrPothosBlock<${namespace}::${factory["className"]}>(__orig_block, ${factory["vlen"]}, ${factory["dtype"]});
    auto __orig_block_ref = std::ref(*static_cast<${namespace}::${factory["className"]} *>(__orig_block.get()));
    % if methods:
    % for method in methods:
    __pothos_block->registerCallable("${method["name"]}", Pothos::Callable(&${namespace}::${factory["className"]}::${method["name"]}).bind(__orig_block_ref, 0));
    % if not method["arguments"] and method["name"] not in ["start", "stop"]:
    __pothos_block->registerProbe("${method["name"]}", "${method["name"]}_triggered", "probe_${method["name"]}");
    % endif
    % endfor
    % endif
    __pothos_block->registerCallable("declare_sample_delay", Pothos::Callable((DeclareSampleDelayPtr)&${namespace}::${factory["className"]}::declare_sample_delay).bind(__orig_block_ref, 0));
    __pothos_block->registerCallable("tag_propagation_policy", Pothos::Callable(&${namespace}::${factory["className"]}::tag_propagation_policy).bind(__orig_block_ref, 0));
    __pothos_block->registerCallable("set_tag_propagation_policy", Pothos::Callable(&${namespace}::${factory["className"]}::set_tag_propagation_policy).bind(__orig_block_ref, 0));
    return __pothos_block;
}

% for ns in namespace.split("::"):
} //namespace $ns
% endfor
% endfor

/***********************************************************************
 * enum conversions
 **********************************************************************/
% for enum in enums:
static ${namespace}::${enum["name"]} int_to_${enum["name"]}(const int v)
{
    return ${namespace}::${enum["name"]}(v);
}
static ${namespace}::${enum["name"]} string_to_${enum["name"]}(const std::string &s)
{
    % for value in enum["values"]:
    % if enum["isEnumClass"]:
    if (s == "${value[0]}") return ${namespace}::${enum["name"]}::${value[0]};
    % else:
    if (s == "${value[0]}") return ${namespace}::${value[0]};
    % endif
    % endfor
    throw Pothos::RuntimeException("convert string to ${enum["name"]} unknown value: "+s);
}
% endfor

/***********************************************************************
 * register block descriptions and conversions
 **********************************************************************/
#include <Pothos/Plugin.hpp>

pothos_static_block(registerGrPothosUtilBlockDocs)
{
    % for enum in enums:
    Pothos::PluginRegistry::add("/object/convert/gr_enums/int_to_${namespace.replace('::', '_')}_${enum["name"]}", Pothos::Callable(&int_to_${enum["name"]}));
    Pothos::PluginRegistry::add("/object/convert/gr_enums/string_to_${namespace.replace('::', '_')}_${enum["name"]}", Pothos::Callable(&string_to_${enum["name"]}));
    % endfor
}
