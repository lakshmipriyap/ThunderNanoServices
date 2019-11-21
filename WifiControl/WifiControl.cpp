#include "WifiControl.h"

namespace WPEFramework {

namespace Plugin
{

    using namespace JsonData::WifiControl;

    SERVICE_REGISTRATION(WifiControl, 1, 0);

    static Core::ProxyPoolType<Web::JSONBodyType<StatusData>> jsonResponseFactoryStatus(1);
    static Core::ProxyPoolType<Web::JSONBodyType<WifiControl::NetworkList>> jsonResponseFactoryNetworkList(1);
    static Core::ProxyPoolType<Web::JSONBodyType<WifiControl::ConfigList>> jsonResponseFactoryConfigList(1);
    static Core::ProxyPoolType<Web::JSONBodyType<JsonData::WifiControl::ConfigInfo>> jsonResponseFactoryConfig(1);

    static string SSIDDecode(const string& item)
    {
        TCHAR converted[256];

        return (string(converted, Core::URL::Decode(item.c_str(), item.length(), converted, (sizeof(converted) / sizeof(TCHAR)))));
    }

    WifiControl::WifiControl()
        : _skipURL(0)
        , _service(nullptr)
        , _configurationStore()
        , _sink(*this)
        , _wpaSupplicant()
        , _controller()
    {
        RegisterAll();
    }

    /* virtual */ const string WifiControl::Initialize(PluginHost::IShell * service)
    {
        ASSERT(_service == nullptr);
        ASSERT(service != nullptr);
        ASSERT(_controller.IsValid() == false);

        string result;
        Config config;
        config.FromString(service->ConfigLine());
        _skipURL = static_cast<uint8_t>(service->WebPrefix().length());
        _service = service;

        if (Core::Directory(service->PersistentPath().c_str()).CreatePath())
            _configurationStore = service->PersistentPath() + "wpa_supplicant.conf";
        else
            SYSLOG(Logging::Startup, ("Config directory %s doesn't exist and could not be created!\n", service->PersistentPath().c_str()));

        TRACE_L1("Starting the application for wifi called: [%s]", config.Application.Value().c_str());
#ifdef USE_WIFI_HAL
        _controller = WPASupplicant::WifiHAL::Create();
        if ((_controller.IsValid() == true) && (_controller->IsOperational() == true)) {
            _controller->Scan();
        }
#else
        if ((config.Application.Value().empty() == false) && (::strncmp(config.Application.Value().c_str(), _TXT("null")) != 0)) {
            if (_wpaSupplicant.Launch(config.Application.Value(), config.Connector.Value(), config.Interface.Value(), 15) != Core::ERROR_NONE) {
                result = _T("Could not start WPA_SUPPLICANT");
            }
        }

        if (result.empty() == true) {
            _controller = WPASupplicant::Controller::Create(config.Connector.Value(), config.Interface.Value(), config.BssExpirationAge.Value(), 10);

            if (_controller.IsValid() == true) {
                if (_controller->IsOperational() == false) {
                    _controller.Release();
                    result = _T("Could not establish a link with WPA_SUPPLICANT");
                } else {
                    _controller->Callback(&_sink);
                    _controller->Scan();

                    Core::File configFile(_configurationStore);

                    if (configFile.Open(true) == true) {
                        ConfigList configs;
                        configs.FromFile(configFile);

                        // iterator over the list and write back
                        auto index(configs.Configs.Elements());

                        while (index.Next()) {

                            WPASupplicant::Config profile(_controller->Create(SSIDDecode(index.Current().Ssid.Value())));

                            ASSERT(index.Current().Ssid.Value().empty() == false);

                            UpdateConfig(profile, index.Current());
                        }
                    }
                }
            }
        }
#endif

        TRACE_L1("Config path = %s", _configurationStore.c_str());

        // On success return empty, to indicate there is no error text.
        return (result);
    }

    /* virtual */ void WifiControl::Deinitialize(PluginHost::IShell * service)
    {
#ifndef USE_WIFI_HAL
        _controller->Callback(nullptr);

        if( _wpaSupplicant.WasStarted() == true ) {
            _controller->Terminate(); 
            _wpaSupplicant.Terminate(); 
        }

        _controller.Release();

#endif

        ASSERT(_service == service);
        _service = nullptr;
    }

    /* virtual */ string WifiControl::Information() const
    {
        // No additional info to report.
        return (string());
    }

    /* virtual */ void WifiControl::Inbound(Web::Request & request)
    {
        Core::TextSegmentIterator index(Core::TextFragment(request.Path, _skipURL,
                                            static_cast<uint32_t>(request.Path.length() - _skipURL)),
            false, '/');

        // By default, we are in front of any element, jump onto the first element, which is if, there is something an empty slot.
        index.Next();

        if ((request.Verb == Web::Request::HTTP_PUT) || (request.Verb == Web::Request::HTTP_POST)) {
            if ((index.IsValid() == true) && (index.Next() == true) && (index.Current().Text() == _T("Config"))) {
                request.Body(jsonResponseFactoryConfig.Element());
            }
        }
    }

    /* virtual */ Core::ProxyType<Web::Response> WifiControl::Process(const Web::Request& request)
    {
        TRACE_L1("Web request %s", request.Path.c_str());
        ASSERT(_skipURL <= request.Path.length());

        Core::ProxyType<Web::Response> result;
        Core::TextSegmentIterator index(Core::TextFragment(request.Path, _skipURL, request.Path.length() - _skipURL), false, '/');
        // By default, we are in front of any element, jump onto the first element, which is if, there is something an empty slot.
        index.Next();
        if (request.Verb == Web::Request::HTTP_GET) {

            result = GetMethod(index);
        } else if (request.Verb == Web::Request::HTTP_PUT) {

            result = PutMethod(index, request);
        } else if (request.Verb == Web::Request::HTTP_POST) {

            result = PostMethod(index, request);
        } else if (request.Verb == Web::Request::HTTP_DELETE) {

            result = DeleteMethod(index);
        }

        return result;
    }

    Core::ProxyType<Web::Response> WifiControl::GetMethod(Core::TextSegmentIterator & index)
    {
        Core::ProxyType<Web::Response> result(PluginHost::Factories::Instance().Response());
        result->ErrorCode = Web::STATUS_BAD_REQUEST;
        result->Message = _T("Unsupported GET request.");

        if (index.IsValid() == true) {
            if (index.Next() && index.IsValid()) {
                if (index.Current().Text() == _T("Networks")) {

                    Core::ProxyType<Web::JSONBodyType<WifiControl::NetworkList>> networks(jsonResponseFactoryNetworkList.Element());
                    WPASupplicant::Network::Iterator list(_controller->Networks());
                    networks->Set(list);

                    result->ErrorCode = Web::STATUS_OK;
                    result->Message = _T("Scanned networks.");
                    result->Body(networks);

                } else if (index.Current().Text() == _T("Configs")) {

                    Core::ProxyType<Web::JSONBodyType<WifiControl::ConfigList>> configs(jsonResponseFactoryConfigList.Element());
                    WPASupplicant::Config::Iterator list(_controller->Configs());
                    configs->Set(list);

                    result->ErrorCode = Web::STATUS_OK;
                    result->Message = _T("Get configurations.");
                    result->Body(configs);

                } else if ((index.Current().Text() == _T("Config")) && (index.Next() == true)) {

                    WPASupplicant::Config entry(_controller->Get(SSIDDecode(index.Current().Text())));
                    if (entry.IsValid() != true) {
                        result->ErrorCode = Web::STATUS_NO_CONTENT;
                        result->Message = _T("Empty config.");
                    } else {
                        Core::ProxyType<Web::JSONBodyType<JsonData::WifiControl::ConfigInfo>> config(jsonResponseFactoryConfig.Element());

                        WifiControl::FillConfig(entry, *config);

                        result->ErrorCode = Web::STATUS_OK;
                        result->Message = _T("Get configuration.");
                        result->Body(config);
                    }
                }
            } else {
                Core::ProxyType<Web::JSONBodyType<StatusData>> status(jsonResponseFactoryStatus.Element());

                result->ErrorCode = Web::STATUS_OK;
                result->Message = _T("Current status.");

                status->Connected = _controller->Current();
                status->Scanning = _controller->IsScanning();

                result->Body(status);
            }
        }

        return result;
    }

    Core::ProxyType<Web::Response> WifiControl::PutMethod(Core::TextSegmentIterator & index, const Web::Request& request)
    {
        Core::ProxyType<Web::Response> result(PluginHost::Factories::Instance().Response());
        result->ErrorCode = Web::STATUS_BAD_REQUEST;
        result->Message = _T("Unsupported PUT request.");

        if (index.IsValid() == true) {
            if (index.Next()) {
                if (index.Current().Text() == _T("Config")) {
                    Core::ProxyType<const JsonData::WifiControl::ConfigInfo> config(request.Body<const JsonData::WifiControl::ConfigInfo>());
                    if (config.IsValid() == false) {
                        result->ErrorCode = Web::STATUS_NO_CONTENT;
                        result->Message = _T("Nothing to set in the config.");
                    } else {

                        WPASupplicant::Config settings(_controller->Create(SSIDDecode(config->Ssid.Value())));

                        UpdateConfig(settings, *config);

                        result->ErrorCode = Web::STATUS_OK;
                        result->Message = _T("Config set.");
                    }
                } else if (index.Current().Text() == _T("Scan")) {

                    result->ErrorCode = Web::STATUS_OK;
                    result->Message = _T("Scan started.");
                    _controller->Scan();

                } else if (index.Current().Text() == _T("Connect")) {
                    if (index.Next() == true) {
                        result->ErrorCode = Web::STATUS_OK;
                        result->Message = _T("Connect started.");

                        _controller->Connect(SSIDDecode(index.Current().Text()));
                    }
                } else if (index.Current().Text() == _T("Store")) {
                    Core::File configFile(_configurationStore);

                    if (configFile.Create() != true) {
                        result->ErrorCode = Web::STATUS_BAD_REQUEST;
                        result->Message = _T("Could not store the information.");

                    } else {
                        result->ErrorCode = Web::STATUS_OK;
                        result->Message = _T("Store started.");

                        WifiControl::ConfigList configs;
                        WPASupplicant::Config::Iterator list(_controller->Configs());
                        configs.Set(list);
                        configs.ToFile(configFile);
                    }

                } else if ((index.Current().Text() == _T("Debug")) && (index.Next() == true)) {
                    string textNumber(index.Current().Text());
                    uint32_t number(Core::NumberType<uint32_t>(Core::TextFragment(textNumber.c_str(), textNumber.length())));
                    result->ErrorCode = Web::STATUS_OK;
                    result->Message = _T("Debug level set.");
                    _controller->Debug(number);
                }
            }
        }
        return result;
    }

    Core::ProxyType<Web::Response> WifiControl::PostMethod(Core::TextSegmentIterator & index, const Web::Request& request)
    {
        Core::ProxyType<Web::Response> result(PluginHost::Factories::Instance().Response());
        result->ErrorCode = Web::STATUS_BAD_REQUEST;
        result->Message = _T("Unsupported POST requestservice.");

        if ((index.IsValid() == true) && (index.Next() && (index.Current().Text() == _T("Config")))) {
            Core::ProxyType<const JsonData::WifiControl::ConfigInfo> config(request.Body<const JsonData::WifiControl::ConfigInfo>());
            if ((config.IsValid() == false) || (config->Ssid.Value().empty() == true)) {
                result->ErrorCode = Web::STATUS_NO_CONTENT;
                result->Message = _T("Nothing to set in the config.");
            } else {

                WPASupplicant::Config settings(_controller->Get(SSIDDecode(config->Ssid.Value())));

                if (settings.IsValid() == false) {
                    result->ErrorCode = Web::STATUS_NOT_FOUND;
                    result->Message = _T("Config key not found.");
                } else {

                    UpdateConfig(settings, *config);

                    result->ErrorCode = Web::STATUS_OK;
                    result->Message = _T("Config set.");
                }
            }
        }

        return result;
    }

    Core::ProxyType<Web::Response> WifiControl::DeleteMethod(Core::TextSegmentIterator & index)
    {
        Core::ProxyType<Web::Response> result(PluginHost::Factories::Instance().Response());
        result->ErrorCode = Web::STATUS_BAD_REQUEST;
        result->Message = _T("Unsupported DELETE request.");

        if (index.IsValid() == true) {
            if (index.Next()) {
                if (index.Current().Text() == _T("Config")) {
                    if (index.Next() == true) {
                        result->ErrorCode = Web::STATUS_OK;
                        result->Message = _T("Config destroyed.");
                        _controller->Destroy(SSIDDecode(index.Current().Text()));
                    }
                } else if (index.Current().Text() == _T("Connect")) {
                    if (index.Next() == true) {
                        result->ErrorCode = Web::STATUS_OK;
                        result->Message = _T("Disconnected.");
                        _controller->Disconnect(SSIDDecode(index.Current().Text()));
                    }
                }
            }
        }

        return result;
    }

    void WifiControl::WifiEvent(const WPASupplicant::Controller::events& event)
    {

        TRACE(Trace::Information, (_T("WPASupplicant notification: %s"), Core::EnumerateType<WPASupplicant::Controller::events>(event).Data()));

        switch (event) {
        case WPASupplicant::Controller::CTRL_EVENT_SCAN_RESULTS: {
            WifiControl::NetworkList networks;
            WPASupplicant::Network::Iterator list(_controller->Networks());

            networks.Set(list);

            event_scanresults(networks.Networks);

            string message;

            networks.ToString(message);

            _service->Notify(message);
            break;
        }
        case WPASupplicant::Controller::CTRL_EVENT_CONNECTED: {
            string message("{ \"event\": \"Connected\", \"ssid\": \"" + _controller->Current() + "\" }");
            _service->Notify(message);
            event_connectionchange(_controller->Current());
            break;
        }
        case WPASupplicant::Controller::CTRL_EVENT_DISCONNECTED: {
            string message("{ \"event\": \"Disconnected\" }");
            _service->Notify(message);
            event_connectionchange(string());
            break;
        }
        case WPASupplicant::Controller::CTRL_EVENT_NETWORK_CHANGED: {
            string message("{ \"event\": \"NetworkUpdate\" }");
            _service->Notify(message);
            event_networkchange();
            break;
        }
        case WPASupplicant::Controller::CTRL_EVENT_BSS_ADDED:
        case WPASupplicant::Controller::CTRL_EVENT_BSS_REMOVED:
        case WPASupplicant::Controller::CTRL_EVENT_TERMINATING:
        case WPASupplicant::Controller::CTRL_EVENT_NETWORK_NOT_FOUND:
        case WPASupplicant::Controller::CTRL_EVENT_SCAN_STARTED:
        case WPASupplicant::Controller::WPS_AP_AVAILABLE:
        case WPASupplicant::Controller::AP_ENABLED:
            break;
        }
    }

} // namespace Plugin
} // namespace WPEFramework
