//the generators werent pulling in the fsm object
//support creating fsm from various args

#include <Pothos/Framework.hpp>
#include <Pothos/Proxy.hpp>
#include <Pothos/Plugin.hpp>
#include <gnuradio/trellis/fsm.h>
#include <Pothos/Managed.hpp>

using namespace gr;

//register constructors from simple types so we can construct FSM from args
static auto managedGrTrellisFSM = Pothos::ManagedClass()
    .registerClass<trellis::fsm>()
    .registerConstructor<trellis::fsm>()
    .registerConstructor<trellis::fsm, int, int, int, const std::vector<int> &, const std::vector<int> &>()
    .registerConstructor<trellis::fsm, const char *>()
    .registerConstructor<trellis::fsm, int, int, const std::vector<int> &>()
    .registerConstructor<trellis::fsm, int, int>()
    .registerConstructor<trellis::fsm, int, int, int>()
    .commit("gr/trellis/fsm");

//converter so FSM can be created from an array of args
static trellis::fsm proxyVectorToGrTrellisFSM(const Pothos::ProxyVector &args)
{
    auto env = Pothos::ProxyEnvironment::make("managed");
    auto cls = env->findProxy("gr/trellis/fsm");
    auto fsm = cls.getHandle()->call("()", args.data(), args.size());
    return fsm;
}

pothos_static_block(registerProxyVectorToGrTrellisFSM)
{
    Pothos::PluginRegistry::add("/object/convert/containers/proxy_vec_to_gr_trellis_fsm", Pothos::Callable(&proxyVectorToGrTrellisFSM));
}
