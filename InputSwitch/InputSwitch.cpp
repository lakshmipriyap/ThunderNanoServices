#include "InputSwitch.h"
#include <interfaces/json/JsonData_InputSwitch.h>

namespace WPEFramework {
namespace Plugin {

    SERVICE_REGISTRATION(InputSwitch, 1, 0);

    static Core::ProxyPoolType<Web::JSONBodyType< Core::JSON::ArrayType < InputSwitch::Data > > > jsonResponseFactory(1);

    /* virtual */ const string InputSwitch::Initialize(PluginHost::IShell* service)
    {
        string message;

        ASSERT(service != nullptr);

        _skipURL = static_cast<uint8_t>(service->WebPrefix().length());

        PluginHost::VirtualInput* handler = PluginHost::InputHandler::Handler();

        PluginHost::IPCUserInput* derived = dynamic_cast<PluginHost::IPCUserInput*>(handler);

        if (handler == nullptr) {
            message = _T("This plugin requires the VirtualInput (IPC Relay) to be instantiated");
        }
        else
        {
            _handler = derived->Inputs();
        }

        // On success return empty, to indicate there is no error text.
        return (EMPTY_STRING);
    }

    /* virtual */ void InputSwitch::Deinitialize(PluginHost::IShell* service)
    {
        _handler.Reset();
    }

    /* virtual */ string InputSwitch::Information() const
    {
        // No additional info to report.
        return (EMPTY_STRING);
    }

    /* virtual */ void InputSwitch::Inbound(Web::Request& /* request */)
    {
    }

    /* virtual */ Core::ProxyType<Web::Response> InputSwitch::Process(const Web::Request& request)
    {
        ASSERT(_skipURL <= request.Path.length());

        Core::ProxyType<Web::Response> result(PluginHost::Factories::Instance().Response());

        // By default, we assume everything works..
        result->ErrorCode = Web::STATUS_OK;
        result->Message = "OK";

        // <GET> - currently, only the GET command is supported, returning system info
        if (request.Verb == Web::Request::HTTP_GET) {

            Core::TextSegmentIterator index(Core::TextFragment(request.Path, _skipURL, static_cast<uint32_t>(request.Path.length()) - _skipURL), false, '/');

            // Always skip the first one, it is an empty part because we start with a '/' if there are more parameters.
            index.Next();

            if (index.Next() == false) {
                Core::ProxyType<Web::JSONBodyType< Core::JSON::ArrayType <Data> > > response(jsonResponseFactory.Element());

                // Insert all channels with there status..
                _handler.Reset();
                while (_handler.Next() == true) {
                    Data& element (response->Add());

                    element.Callsign = _handler.Name();
                    element.Enabled = _handler.Enabled();
                }

                result->ContentType = Web::MIMETypes::MIME_JSON;
                result->Body(Core::proxy_cast<Web::IBody>(response));
            }
            else {
                if (FindChannel(index.Current().Text()) == false) {
                    result->ErrorCode = Web::STATUS_MOVED_TEMPORARY;
                    result->Message = _T("Could not find the inidicated channel.");
                }
                else {
                    Core::ProxyType<Web::JSONBodyType< Core::JSON::ArrayType <Data> > > response(jsonResponseFactory.Element());

                    // Insert the requested channel with its status..
                    Data& element(response->Add());

                    element.Callsign = _handler.Name();
                    element.Enabled = _handler.Enabled();

                    // Make sure we reset the handler, otherwise we maintain  
                    // a reference to the selected channel!!
                    _handler.Reset();

                    result->ContentType = Web::MIMETypes::MIME_JSON;
                    result->Body(Core::proxy_cast<Web::IBody>(response));
                }
            }
        }
        else if (request.Verb == Web::Request::HTTP_PUT) {
            Core::TextSegmentIterator index(Core::TextFragment(request.Path, _skipURL, static_cast<uint32_t>(request.Path.length()) - _skipURL), false, '/');

            // Always skip the first one, it is an empty part because we start with a '/' if there are more parameters.
            index.Next();

            if (index.Next() == false) {
                result->ErrorCode = Web::STATUS_BAD_REQUEST;
                result->Message = _T("Need at least a channel name and a requested state.");
            }
            else if (FindChannel(index.Current().Text()) == false) {
                result->ErrorCode = Web::STATUS_MOVED_TEMPORARY;
                result->Message = _T("Could not find the indicated channel.");
            }
            else if (index.Next() == false) {
                result->ErrorCode = Web::STATUS_BAD_REQUEST;
                result->Message = _T("Need at least a state wich is applicable to the channel.");
            }
            else if ((index.Remainder() != _T("On")) || (index.Remainder() != _T("Off"))) {
                result->ErrorCode = Web::STATUS_BAD_REQUEST;
                result->Message = _T("The requested state should be <On> or <Off>.");
            }
            else {
                Core::ProxyType<Web::JSONBodyType< Core::JSON::ArrayType <Data> > > response(jsonResponseFactory.Element());

                // Insert the requested channel with its status..
                Data& element(response->Add());

                _handler.Enable((index.Remainder() != _T("On")));

                element.Callsign = _handler.Name();
                element.Enabled = _handler.Enabled();

                // Make sure we reset the handler, otherwise we maintain  
                // a reference to the selected channel!!
                _handler.Reset();

                result->ContentType = Web::MIMETypes::MIME_JSON;
                result->Body(Core::proxy_cast<Web::IBody>(response));
            }
        } else {
            result->ErrorCode = Web::STATUS_BAD_REQUEST;
            result->Message = _T("Unsupported request for the [InputSwitch] service.");
        }

        return result;
    }

    bool InputSwitch::FindChannel(const string& name) {
        _handler.Reset();
        while ((_handler.Next() == true) && (_handler.Name() != name)) /* Intetionally left empty */;
        return (_handler.IsValid());
    }


} // namespace Plugin
} // namespace WPEFramework
