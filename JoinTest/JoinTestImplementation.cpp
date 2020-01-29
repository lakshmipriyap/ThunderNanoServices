#include "Module.h"
#include <core/Timer.h>

namespace WPEFramework {
namespace Plugin {

    class JoinTestImplementation : public Core::IUnknown {
    public:
        JoinTestImplementation(const JoinTestImplementation&) = delete;
        JoinTestImplementation& operator=(const JoinTestImplementation&) = delete;

        JoinTestImplementation()
            : _libHandle(nullptr)
        {
            _libHandle = dlopen("libexampleTest.so", RTLD_LAZY);

            if (!_libHandle) {
                fprintf(stderr, "\n -----------# JoinTest : Cannot open library libexampleTest.so \n\n");
            }
        }

        virtual ~JoinTestImplementation()
        {
            dlclose(_libHandle);
        };

        BEGIN_INTERFACE_MAP(JoinTestImplementation)
        INTERFACE_ENTRY(Core::IUnknown)
        END_INTERFACE_MAP

    private:
        void* _libHandle;
    };

    SERVICE_REGISTRATION(JoinTestImplementation, 1, 0);

} // namespace Plugin
} // namespace WPEFramework
