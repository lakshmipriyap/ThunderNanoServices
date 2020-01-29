#include <core/Thread.h>

class ExampleClass {
public:
    class WorkerThread : public WPEFramework::Core::Thread {
    public:
        uint32_t Worker() override
        {
            // This probably won't be called, as is the case in OCDM.
            fprintf(stderr, "\n -----------# JoinTest : WorkerThread sleeping for 2s.\n");
            sleep(2);
            return WPEFramework::Core::infinite;
        };

        ~WorkerThread()
        {
            // Past this point, a destructor for Thread object will be called.
            // If there is an issue with pthread_join on a thread that exits with pthread_exit,
            // a JoinTest WPEProcess will still be in the process list.
            fprintf(stderr, "\n\n -----------# JoinTest: WorkerThread destructor\n\n");
        }
    };

public:
    ExampleClass()
        : _exampleThread()
    {
        _exampleThread.Block();
    }

    ~ExampleClass()
    {
        _exampleThread.Stop();
        _exampleThread.Wait(WPEFramework::Core::Thread::BLOCKED | WPEFramework::Core::Thread::STOPPED, WPEFramework::Core::infinite);
    }

private:
    WorkerThread _exampleThread;
};