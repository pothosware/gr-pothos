//unlike the other files,
//this one is done by hand to include runtime definitions
//because we don't normally scan runtime for blocks

#include <Pothos/Framework.hpp>
#include <Pothos/Proxy.hpp>
#include <gnuradio/block.h>

using namespace gr;

/***********************************************************************
 * include block definitions
 **********************************************************************/

/***********************************************************************
 * enum conversions
 **********************************************************************/
gr::block::tag_propagation_policy_t int_to_tag_propagation_policy_t(const int v)
{
    return gr::block::tag_propagation_policy_t(v);
}
gr::block::tag_propagation_policy_t string_to_tag_propagation_policy_t(const std::string &s)
{
    if (s == "TPP_DONT") return gr::block::TPP_DONT;
    if (s == "TPP_ALL_TO_ALL") return gr::block::TPP_ALL_TO_ALL;
    if (s == "TPP_ONE_TO_ONE") return gr::block::TPP_ONE_TO_ONE;
    throw Pothos::RuntimeException("convert string to tag_propagation_policy_t unknown value: "+s);
}

/***********************************************************************
 * register block descriptions and conversions
 **********************************************************************/
#include <Pothos/Plugin.hpp>

pothos_static_block(registerGrPothosUtilRuntimeDocs)
{
    Pothos::PluginRegistry::add("/object/convert/gr_enums/int_to_tag_propagation_policy_t", Pothos::Callable(&int_to_tag_propagation_policy_t));
    Pothos::PluginRegistry::add("/object/convert/gr_enums/string_to_tag_propagation_policy_t", Pothos::Callable(&string_to_tag_propagation_policy_t));
}
