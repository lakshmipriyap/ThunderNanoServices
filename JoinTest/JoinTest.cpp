#include "JoinTest.h"

namespace WPEFramework {
namespace Plugin {
    SERVICE_REGISTRATION(JoinTest, 1, 0);

    /* virtual */ const string JoinTest::Initialize(PluginHost::IShell* service)
    {
        string message = EMPTY_STRING;
        Config config;

        ASSERT(service != nullptr);
        ASSERT(_service == nullptr);
        ASSERT(_joinTestImp == nullptr);
        ASSERT(_connection == 0);

        _service = service;

        _joinTestImp = _service->Root<Core::IUnknown>(_connection, 2000, _T("JoinTestImplementation"));
        ASSERT(_joinTestImp != nullptr);
        return message;
    }

    /* virtual */ void JoinTest::Deinitialize(PluginHost::IShell* service)
    {
        ASSERT(_service == service);
        ASSERT(_joinTestImp != nullptr);
        
        if (_joinTestImp->Release() != Core::ERROR_DESTRUCTION_SUCCEEDED) {
            TRACE(Trace::Information, (_T("JoinTest is not properly destructed (connection=%d)"), _connection));
        }

        _joinTestImp = nullptr;
        _service = nullptr;
    }

    /* virtual */ string JoinTest::Information() const
    {
        return ((_T("The purpose of this plugin is to illustrate a point with dlclose and pthread_exit.")));
    }

} // namespace Plugin
} // namespace WPEFramework
