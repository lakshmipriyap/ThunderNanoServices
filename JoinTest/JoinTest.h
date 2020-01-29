#pragma once

#include "Module.h"

namespace WPEFramework {
namespace Plugin {

    class JoinTest : public PluginHost::IPlugin {
    private:
        JoinTest(const JoinTest&);
        JoinTest& operator=(const JoinTest&);

    public:
        JoinTest()
            : _service(nullptr)
            , _joinTestImp(nullptr)
            , _connection(0)
        {
        }

        virtual ~JoinTest()
        {
        }

        BEGIN_INTERFACE_MAP(JoinTest)
        INTERFACE_ENTRY(PluginHost::IPlugin)
        END_INTERFACE_MAP


        virtual const string Initialize(PluginHost::IShell* service);
        virtual void Deinitialize(PluginHost::IShell* service);
        virtual string Information() const;

    private:
        PluginHost::IShell* _service;
        Core::IUnknown* _joinTestImp;
        uint32_t _connection;
    };
}
}
