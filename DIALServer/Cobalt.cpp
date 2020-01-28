#include "DIALServer.h"

namespace WPEFramework {
namespace DIALHandlers {

    class Cobalt : public Plugin::DIALServer::Default {
    private:
        Cobalt() = delete;
        Cobalt(const Cobalt&) = delete;
        Cobalt& operator=(const Cobalt&) = delete;

    public:
        Cobalt(PluginHost::IShell* service, const Plugin::DIALServer::Config::App& config, Plugin::DIALServer* parent)
            : Default(service, config, parent)
        {
        }

        ~Cobalt() override
        {
        }

    public:
        uint32_t Start(const string& params) override {
            Core::SystemInfo::SetEnvironment(_T("COBALT_START_URL"), params.c_str());
            return Default::Start(params);
        }

    };

    static Plugin::DIALServer::ApplicationRegistrationType<Cobalt> g_cobaltHandler;

}
}
