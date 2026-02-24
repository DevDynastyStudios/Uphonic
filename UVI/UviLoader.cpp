#include "UviLoader.h"

#include "pluginterfaces/vst/ivstaudioprocessor.h"
#include "pluginterfaces/vst/ivstcomponent.h"
#include "pluginterfaces/vst/ivsteditcontroller.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "pluginterfaces/vst/ivstevents.h"
#include "pluginterfaces/vst/ivsthostapplication.h"
#include "pluginterfaces/gui/iplugview.h"
#include "pluginterfaces/vst/ivstmessage.h"
#include "public.sdk/source/vst/hosting/module.h"
#include "public.sdk/source/vst/hosting/hostclasses.h"
#include "public.sdk/source/vst/hosting/parameterchanges.h"
#include "public.sdk/source/common/memorystream.h"

#include <iostream>
#include <fstream>
#include <cassert>
#include <cstring>
#include <cmath>
#include <atomic>
#include <algorithm>
#include <chrono>
#include <mutex>
#include <map>

#if defined(_WIN32)
#include <windows.h>
#elif defined(__linux__)
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <sys/select.h>
#endif

using namespace Steinberg;
using namespace Steinberg::Vst;

namespace Uvi
{

// ---------------------------------------------------------------------------
// Module cache — intentionally never unloaded.
//
// Many complex plugins (Kontakt, Guitar Rig, etc.) spin up background threads
// and register static destructors that are not safe to run while the host is
// still alive. Calling FreeLibrary / dlclose on them causes crashes. The
// industry-standard workaround is to keep every loaded module resident for
// the lifetime of the process. Memory cost is negligible compared to the
// plugin's own footprint, and the OS reclaims everything on exit anyway.
// ---------------------------------------------------------------------------
static std::mutex                                        s_moduleCacheMutex;
static std::map<std::string, VST3::Hosting::Module::Ptr> s_moduleCache;

#define LOG_DEBUG(msg) std::cout << "[DEBUG] " << msg << '\n'

// Enhanced host application - handles both IMessage and IAttributeList creation
class MyHostApplication : public IHostApplication
{
public:
    MyHostApplication() : refCount(1) {}
    virtual ~MyHostApplication() {}

    tresult PLUGIN_API queryInterface(const TUID _iid, void** obj) override
    {
        QUERY_INTERFACE(_iid, obj, FUnknown::iid, IHostApplication)
        QUERY_INTERFACE(_iid, obj, IHostApplication::iid, IHostApplication)
        *obj = nullptr;
        return kNoInterface;
    }

    uint32 PLUGIN_API addRef() override { return ++refCount; }

    uint32 PLUGIN_API release() override
    {
        if (--refCount == 0)
        {
            delete this;
            return 0;
        }
        return refCount;
    }

    tresult PLUGIN_API getName(String128 name) override
    {
        char16_t appName[] = u"Uvi VST3 Host";
        memcpy(name, appName, sizeof(appName));
        return kResultOk;
    }

    tresult PLUGIN_API createInstance(TUID cid, TUID _iid, void** obj) override
    {
        FUID classID(FUID::fromTUID(cid));
        FUID interfaceID(FUID::fromTUID(_iid));

        if (classID == IMessage::iid && interfaceID == IMessage::iid)
        {
            *obj = new HostMessage;
            return kResultOk;
        }

        if (classID == IAttributeList::iid && interfaceID == IAttributeList::iid)
        {
            *obj = nullptr;
            return kResultFalse;
        }

        *obj = nullptr;
        return kResultFalse;
    }

private:
    std::atomic<int32> refCount;
};

class PlugFrame : public IPlugFrame
#if defined(__linux__)
    , public Steinberg::Linux::IRunLoop
#endif
{
public:
#if defined(_WIN32)
    PlugFrame(void* handle) : nativeHandle(static_cast<HWND>(handle)), refCount(1) {}
#elif defined(__linux__)
    PlugFrame(void* handle)
        : nativeHandle(reinterpret_cast<Window>(reinterpret_cast<uintptr_t>(handle)))
        , refCount(1)
    {}
#else
    PlugFrame(void* handle) : nativeHandle(handle), refCount(1) {}
#endif

    virtual ~PlugFrame() {}

    // -------------------------------------------------------------------------
    // IPlugFrame
    // -------------------------------------------------------------------------
    Steinberg::tresult PLUGIN_API resizeView(Steinberg::IPlugView* view, Steinberg::ViewRect* newSize) override
    {
        if (!newSize)
            return Steinberg::kResultFalse;

#if defined(_WIN32)
        if (nativeHandle)
        {
            RECT rect = {0, 0, newSize->getWidth(), newSize->getHeight()};
            AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);

            SetWindowPos(nativeHandle, NULL, 0, 0,
                         rect.right - rect.left,
                         rect.bottom - rect.top,
                         SWP_NOMOVE | SWP_NOZORDER);
        }
#elif defined(__linux__)
        if (nativeHandle)
        {
            Display* display = XOpenDisplay(nullptr);
            if (display)
            {
                XResizeWindow(display, nativeHandle,
                              (unsigned int)newSize->getWidth(),
                              (unsigned int)newSize->getHeight());
                XFlush(display);
                XCloseDisplay(display);
            }
        }
#endif

        if (view)
            view->onSize(newSize);

        return Steinberg::kResultOk;
    }

    // -------------------------------------------------------------------------
    // IRunLoop (Linux only)
    //
    // Plugins register their X11 file descriptor handlers and UI timers here.
    // Instead of spinning up real OS timers, we drive everything from the
    // host's existing render loop via RunIdle(), which Vst3Plugin::IdleEditor()
    // calls every frame.
    // -------------------------------------------------------------------------
#if defined(__linux__)
    struct EventHandlerEntry
    {
        Steinberg::Linux::IEventHandler* handler;
        Steinberg::Linux::FileDescriptor fd;
    };

    struct TimerEntry
    {
        Steinberg::Linux::ITimerHandler*       handler;
        Steinberg::Linux::TimerInterval        intervalMs;
        std::chrono::steady_clock::time_point  lastFired;
    };

    tresult PLUGIN_API registerEventHandler(Steinberg::Linux::IEventHandler* handler,
                                            Steinberg::Linux::FileDescriptor fd) override
    {
        if (!handler)
            return kInvalidArgument;
        m_eventHandlers.push_back({handler, fd});
        return kResultOk;
    }

    tresult PLUGIN_API unregisterEventHandler(Steinberg::Linux::IEventHandler* handler) override
    {
        if (!handler)
            return kInvalidArgument;
        m_eventHandlers.erase(
            std::remove_if(m_eventHandlers.begin(), m_eventHandlers.end(),
                [handler](const EventHandlerEntry& e) { return e.handler == handler; }),
            m_eventHandlers.end());
        return kResultOk;
    }

    tresult PLUGIN_API registerTimer(Steinberg::Linux::ITimerHandler* handler,
                                     Steinberg::Linux::TimerInterval milliseconds) override
    {
        if (!handler)
            return kInvalidArgument;
        m_timers.push_back({handler, milliseconds, std::chrono::steady_clock::now()});
        return kResultOk;
    }

    tresult PLUGIN_API unregisterTimer(Steinberg::Linux::ITimerHandler* handler) override
    {
        if (!handler)
            return kInvalidArgument;
        m_timers.erase(
            std::remove_if(m_timers.begin(), m_timers.end(),
                [handler](const TimerEntry& e) { return e.handler == handler; }),
            m_timers.end());
        return kResultOk;
    }

    // Called every frame from Vst3Plugin::IdleEditor().
    // Polls registered FDs (non-blocking) and fires any timers that are due.
    void RunIdle()
    {
        // Poll each registered file descriptor and notify the handler if ready
        for (auto& entry : m_eventHandlers)
        {
            fd_set fds;
            FD_ZERO(&fds);
            FD_SET(entry.fd, &fds);
            timeval tv = {0, 0}; // non-blocking poll
            if (select(entry.fd + 1, &fds, nullptr, nullptr, &tv) > 0)
                entry.handler->onFDIsSet(entry.fd);
        }

        // Fire timers that have elapsed
        auto now = std::chrono::steady_clock::now();
        for (auto& entry : m_timers)
        {
            auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                                 now - entry.lastFired).count();
            if (elapsedMs >= static_cast<long long>(entry.intervalMs))
            {
                entry.handler->onTimer();
                entry.lastFired = now;
            }
        }
    }

private:
    std::vector<EventHandlerEntry> m_eventHandlers;
    std::vector<TimerEntry>        m_timers;

public:
#endif // __linux__

    // -------------------------------------------------------------------------
    // FUnknown
    // -------------------------------------------------------------------------
    Steinberg::tresult PLUGIN_API queryInterface(const TUID _iid, void** obj) override
    {
        QUERY_INTERFACE(_iid, obj, Steinberg::FUnknown::iid, Steinberg::IPlugFrame)
        QUERY_INTERFACE(_iid, obj, Steinberg::IPlugFrame::iid, Steinberg::IPlugFrame)
#if defined(__linux__)
        QUERY_INTERFACE(_iid, obj, Steinberg::Linux::IRunLoop::iid, Steinberg::Linux::IRunLoop)
#endif
        *obj = nullptr;
        return Steinberg::kNoInterface;
    }

    Steinberg::uint32 PLUGIN_API addRef() override { return ++refCount; }

    Steinberg::uint32 PLUGIN_API release() override
    {
        if (--refCount == 0)
        {
            delete this;
            return 0;
        }
        return refCount;
    }

private:
#if defined(_WIN32)
    HWND nativeHandle;
#elif defined(__linux__)
    Window nativeHandle;
#else
    void* nativeHandle;
#endif
    Steinberg::int32 refCount;
};

class EventList : public IEventList
{
public:
    EventList() : refCount(1) {}
    virtual ~EventList() {}

    int32 PLUGIN_API getEventCount() override { return (int32)events.size(); }

    tresult PLUGIN_API getEvent(int32 index, Event& e) override
    {
        if (index >= 0 && index < (int32)events.size())
        {
            e = events[index];
            return kResultOk;
        }
        return kResultFalse;
    }

    tresult PLUGIN_API addEvent(Event& e) override
    {
        events.push_back(e);
        return kResultOk;
    }

    void clear() { events.clear(); }

    tresult PLUGIN_API queryInterface(const TUID _iid, void** obj) override
    {
        QUERY_INTERFACE(_iid, obj, FUnknown::iid, IEventList)
        QUERY_INTERFACE(_iid, obj, IEventList::iid, IEventList)
        *obj = nullptr;
        return kNoInterface;
    }

    uint32 PLUGIN_API addRef() override { return ++refCount; }

    uint32 PLUGIN_API release() override
    {
        if (--refCount == 0) {
            delete this;
            return 0;
        }
        return refCount;
    }

private:
    std::vector<Event> events;
    int32 refCount;
};

// Component handler implementation
class ComponentHandler : public IComponentHandler
{
public:
    ComponentHandler(IComponent* component, ParameterChanges* inputChanges)
        : m_component(component)
        , m_inputChanges(inputChanges)
        , refCount(1)
    {
    }

    virtual ~ComponentHandler() {}

    tresult PLUGIN_API beginEdit(ParamID id) override
    {
        (void)id;
        return kResultOk;
    }

    tresult PLUGIN_API performEdit(ParamID id, ParamValue valueNormalized) override
    {
        if (m_inputChanges)
        {
            int32 index = 0;
            IParamValueQueue* queue = m_inputChanges->addParameterData(id, index);
            if (queue)
                queue->addPoint(0, valueNormalized, index);
        }
        return kResultOk;
    }

    tresult PLUGIN_API endEdit(ParamID id) override
    {
        (void)id;
        return kResultOk;
    }

    tresult PLUGIN_API restartComponent(int32 flags) override
    {
        if (flags & kReloadComponent)
            LOG_DEBUG("Plugin requests component reload");
        if (flags & kIoChanged)
            LOG_DEBUG("Plugin requests I/O reconfiguration");
        if (flags & kLatencyChanged)
            LOG_DEBUG("Plugin reports latency change");
        return kResultOk;
    }

    tresult PLUGIN_API queryInterface(const TUID _iid, void** obj) override
    {
        QUERY_INTERFACE(_iid, obj, FUnknown::iid, IComponentHandler)
        QUERY_INTERFACE(_iid, obj, IComponentHandler::iid, IComponentHandler)
        *obj = nullptr;
        return kNoInterface;
    }

    uint32 PLUGIN_API addRef() override { return ++refCount; }

    uint32 PLUGIN_API release() override
    {
        if (--refCount == 0)
        {
            delete this;
            return 0;
        }
        return refCount;
    }

private:
    IComponent*        m_component    = nullptr;
    ParameterChanges*  m_inputChanges = nullptr;
    std::atomic<int32> refCount;
};

class Vst3Plugin : public Plugin
{
public:
    Vst3Plugin(const char* path, float sampleRate, int32_t blockSize);
    ~Vst3Plugin();

    void AttachEditor(void* handle) override;
    void DetachEditor() override;
    void IdleEditor() override;
    void Process(float** inputs, float** outputs, int32_t sampleFrames, float bpm) override;
    void PlayNote(int32_t key, int32_t velocity, int32_t sampleOffset) override;
    void StopNote(int32_t key, int32_t sampleOffset) override;
    void StopAllNotes() override;
    void Serialize(const char* filePath) override;
    void Deserialize(const char* filePath) override;
    bool IsLoaded() const override { return m_isLoaded; }
    const char* GetName() const override { return m_name.c_str(); }
    const char* GetVendor() const override { return m_vendor.c_str(); }
    const char* GetVersion() const override { return m_version.c_str(); }
    const char* GetUID() const override { return m_uid.c_str(); }

private:
    void SetupProcessing(double sampleRate, int32_t maxBlockSize);
    bool InitializeComponent(IComponent* component, const VST3::Hosting::ClassInfo& classInfo,
                             const VST3::Hosting::PluginFactory& factory);
    void CleanupProcessing();
    void CleanupEditor();
    void CleanupController();
    void CleanupComponent();

    FUnknownPtr<IComponent>         m_component;
    FUnknownPtr<IAudioProcessor>    m_processor;
    FUnknownPtr<IEditController>    m_controller;
    FUnknownPtr<IPlugView>          m_plugView;
    FUnknownPtr<IConnectionPoint>   m_componentCP;
    FUnknownPtr<IConnectionPoint>   m_controllerCP;
    FUnknownPtr<IHostApplication>   m_hostAppComponent;
    FUnknownPtr<IHostApplication>   m_hostAppController;
    FUnknownPtr<IComponentHandler>  m_componentHandler;

    PlugFrame*  m_plugFrame = nullptr;
    EventList*  m_eventList = nullptr;

    ProcessData     processData;
    ProcessContext  processContext;
    AudioBusBuffers inputBuses[2];
    AudioBusBuffers outputBuses[2];

    ParameterChanges inputParamChanges;
    ParameterChanges outputParamChanges;

    std::string m_name;
    std::string m_vendor;
    std::string m_version;
    std::string m_uid;
    bool m_isLoaded             = false;
    bool m_processingActive     = false;
    bool m_componentActive      = false;
    bool m_isControllerSeparate = false;

    int32_t m_numInputBuses  = 0;
    int32_t m_numOutputBuses = 0;
    int32_t m_inputChannels  = 0;
    int32_t m_outputChannels = 0;
    double  m_sampleRate     = 0.0;
    int32_t m_maxBlockSize   = 0;

    // Per-bus channel offsets into the flat inputs/outputs pointer arrays.
    // Computed once in SetupProcessing and used in Process to correctly
    // address each bus regardless of how many channels the previous bus has.
    int32_t m_inputBusChannelOffset[2]  = {0, 0};
    int32_t m_outputBusChannelOffset[2] = {0, 0};

    VST3::Hosting::Module::Ptr m_module;
};

Vst3Plugin::Vst3Plugin(const char* path, float sampleRate, int32_t blockSize)
{
    LOG_DEBUG("=== VST3 Plugin Loading Started ===");
    LOG_DEBUG("Path: " << path);
    LOG_DEBUG("Sample Rate: " << sampleRate << ", Block Size: " << blockSize);

    std::string errorMsg;
    LOG_DEBUG("Creating VST3 module...");

    {
        std::lock_guard<std::mutex> lock(s_moduleCacheMutex);
        auto it = s_moduleCache.find(std::string(path));
        if (it != s_moduleCache.end())
        {
            m_module = it->second;
            LOG_DEBUG("Module retrieved from cache");
        }
        else
        {
            m_module = VST3::Hosting::Module::create(path, errorMsg);
            if (m_module)
            {
                s_moduleCache[std::string(path)] = m_module;
                LOG_DEBUG("Module created and cached");
            }
        }
    }

    if (!m_module)
    {
        std::cerr << "Failed to create VST3 module: " << errorMsg << std::endl;
        return;
    }
    LOG_DEBUG("Module created successfully");

    LOG_DEBUG("Getting factory...");
    auto& factory = m_module->getFactory();
    const auto& classInfos = factory.classInfos();
    LOG_DEBUG("Found " << classInfos.size() << " class(es) in factory");

    int classIndex = 0;
    for (const auto& classInfo : classInfos)
    {
        LOG_DEBUG("Class " << classIndex++ << ": " << classInfo.name()
                           << " (category: " << classInfo.category() << ")");

        if (classInfo.category() == kVstAudioEffectClass)
        {
            m_name    = classInfo.name();
            m_vendor  = classInfo.vendor();
            m_version = classInfo.version();
            m_uid     = classInfo.ID().toString();

            LOG_DEBUG("Found audio effect: " << m_name);
            LOG_DEBUG("Creating component instance...");

            auto component = factory.createInstance<IComponent>(classInfo.ID());
            if (!component)
            {
                std::cerr << "Failed to create component instance" << std::endl;
                continue;
            }
            LOG_DEBUG("Component instance created");

            if (InitializeComponent(component, classInfo, factory))
            {
                m_isLoaded = true;
                LOG_DEBUG("Component initialized successfully");
                break;
            }
            else
            {
                LOG_DEBUG("Component initialization failed");
                continue;
            }
        }
    }

    if (!m_isLoaded)
    {
        std::cerr << "Failed to initialize any plugin component" << std::endl;
        return;
    }

    LOG_DEBUG("Initializing process data structures...");

    m_eventList = new EventList();

    memset(&processData,    0, sizeof(ProcessData));
    memset(&processContext, 0, sizeof(ProcessContext));

    processData.processMode        = Steinberg::Vst::kRealtime;
    processData.symbolicSampleSize = Steinberg::Vst::kSample32;
    processData.numSamples         = 0;
    processData.numInputs          = 0;
    processData.numOutputs         = 0;
    processData.inputs             = inputBuses;
    processData.outputs            = outputBuses;

    processContext.state =
        ProcessContext::kPlaying               |
        ProcessContext::kSystemTimeValid       |
        ProcessContext::kContTimeValid         |
        ProcessContext::kProjectTimeMusicValid |
        ProcessContext::kBarPositionValid      |
        ProcessContext::kCycleValid            |
        ProcessContext::kTempoValid            |
        ProcessContext::kTimeSigValid;
    processContext.sampleRate           = sampleRate;
    processContext.projectTimeSamples   = 0;
    processContext.systemTime           = 0;
    processContext.continousTimeSamples = 0;
    processContext.projectTimeMusic     = 0.0;
    processContext.barPositionMusic     = 0.0;
    processContext.cycleStartMusic      = 0.0;
    processContext.cycleEndMusic        = 4.0;
    processContext.tempo                = 120.0;
    processContext.timeSigNumerator     = 4;
    processContext.timeSigDenominator   = 4;
    memset(&processContext.chord, 0, sizeof(processContext.chord));
    processContext.smpteOffsetSubframes      = 0;
    processContext.frameRate.framesPerSecond = 0;
    processContext.frameRate.flags           = 0;
    processContext.samplesToNextClock        = 0;

    processData.processContext          = &processContext;
    processData.inputParameterChanges  = &inputParamChanges;
    processData.outputParameterChanges = &outputParamChanges;
    processData.inputEvents            = m_eventList;
    processData.outputEvents           = nullptr;

    memset(inputBuses,  0, sizeof(inputBuses));
    memset(outputBuses, 0, sizeof(outputBuses));

    LOG_DEBUG("Calling SetupProcessing...");
    SetupProcessing(sampleRate, blockSize);
    LOG_DEBUG("=== VST3 Plugin Loading Complete ===");
}

bool Vst3Plugin::InitializeComponent(IComponent* component, const VST3::Hosting::ClassInfo& classInfo,
                                      const VST3::Hosting::PluginFactory& factory)
{
    LOG_DEBUG("InitializeComponent started");

    m_component = component;

    LOG_DEBUG("Creating HostApplication (component)...");
    m_hostAppComponent = new MyHostApplication();

    LOG_DEBUG("Calling component->initialize()...");
    if (m_component->initialize(m_hostAppComponent) != kResultOk)
    {
        std::cerr << "Failed to initialize component" << std::endl;
        m_hostAppComponent = nullptr;
        m_component        = nullptr;
        return false;
    }
    LOG_DEBUG("Component initialized");

    LOG_DEBUG("Getting IAudioProcessor interface...");
    m_processor = FUnknownPtr<IAudioProcessor>(m_component);
    if (!m_processor)
    {
        std::cerr << "Component does not support IAudioProcessor interface" << std::endl;
        m_component->terminate();
        m_component = nullptr;
        return false;
    }
    LOG_DEBUG("Got IAudioProcessor interface");

    LOG_DEBUG("Getting controller class ID...");
    TUID controllerCID;
    memset(controllerCID, 0, sizeof(TUID));

    if (m_component->getControllerClassId(controllerCID) == kResultOk)
    {
        bool isNullTUID = true;
        for (int i = 0; i < 16; i++)
        {
            if (controllerCID[i] != 0)
            {
                isNullTUID = false;
                break;
            }
        }

        if (!isNullTUID)
        {
            LOG_DEBUG("Attempting to create separate controller...");
            auto controller = factory.createInstance<IEditController>(controllerCID);
            if (controller)
            {
                LOG_DEBUG("Controller instance created, initializing...");
                m_hostAppController = new MyHostApplication();

                if (controller->initialize(m_hostAppController) == kResultOk)
                {
                    LOG_DEBUG("Controller initialized");
                    m_controller           = controller;
                    m_isControllerSeparate = true;

                    m_componentHandler = new ComponentHandler(m_component, &inputParamChanges);
                    m_controller->setComponentHandler(m_componentHandler);

                    LOG_DEBUG("Connecting component and controller...");
                    m_componentCP  = FUnknownPtr<IConnectionPoint>(m_component);
                    m_controllerCP = FUnknownPtr<IConnectionPoint>(m_controller);

                    if (m_componentCP && m_controllerCP)
                    {
                        m_componentCP->connect(m_controllerCP);
                        m_controllerCP->connect(m_componentCP);
                        LOG_DEBUG("Connected component and controller");
                    }

                    LOG_DEBUG("Syncing component state to controller...");
                    MemoryStream stream;
                    if (m_component->getState(&stream) == kResultOk)
                    {
                        stream.seek(0, IBStream::kIBSeekSet, nullptr);
                        m_controller->setComponentState(&stream);
                        LOG_DEBUG("State synced");
                    }

                    std::cout << "Loaded plugin with separate controller: " << m_name << std::endl;
                    return true;
                }
                else
                {
                    std::cerr << "Failed to initialize separate controller" << std::endl;
                    m_hostAppController = nullptr;
                }
            }
            else
            {
                std::cerr << "Failed to create separate controller instance" << std::endl;
            }
        }
        else
        {
            LOG_DEBUG("Controller TUID is null");
        }
    }

    LOG_DEBUG("Trying single-component plugin...");
    m_controller = FUnknownPtr<IEditController>(m_component);
    if (m_controller)
    {
        m_isControllerSeparate = false;

        m_componentHandler = new ComponentHandler(m_component, &inputParamChanges);
        m_controller->setComponentHandler(m_componentHandler);

        std::cout << "Loaded single-component plugin: " << m_name << std::endl;
        return true;
    }

    std::cerr << "Warning: Plugin has no controller interface - limited functionality" << std::endl;
    std::cout << "Loaded plugin without controller: " << m_name << std::endl;
    return true;
}

void Vst3Plugin::CleanupEditor()
{
    LOG_DEBUG("CleanupEditor started");

    if (m_plugView)
    {
        LOG_DEBUG("Removing view...");
        m_plugView->setFrame(nullptr);
        m_plugView->removed();
        m_plugView = nullptr;
        LOG_DEBUG("View released");
    }

    if (m_plugFrame)
    {
        m_plugFrame->release();
        m_plugFrame = nullptr;
        LOG_DEBUG("Frame released");
    }
}

void Vst3Plugin::CleanupProcessing()
{
    LOG_DEBUG("CleanupProcessing started");

    if (m_processingActive && m_processor)
    {
        LOG_DEBUG("Stopping processing...");
        m_processor->setProcessing(false);
        m_processingActive = false;

        if (m_outputChannels > 0 && m_maxBlockSize > 0)
        {
            LOG_DEBUG("Processing silent blocks for cleanup...");
            std::vector<std::vector<float>> silentBuffers(m_outputChannels,
                                                          std::vector<float>(m_maxBlockSize, 0.0f));
            std::vector<float*> silentPtrs(m_outputChannels);
            for (int32_t i = 0; i < m_outputChannels; i++)
                silentPtrs[i] = silentBuffers[i].data();

            processData.numSamples = m_maxBlockSize;
            for (int i = 0; i < 3; i++)
                m_processor->process(processData);

            LOG_DEBUG("Silent blocks processed");
        }
    }

    if (m_componentActive && m_component)
    {
        LOG_DEBUG("Deactivating component...");
        m_component->setActive(false);
        m_componentActive = false;
    }
}

void Vst3Plugin::CleanupController()
{
    LOG_DEBUG("CleanupController started");

    if (!m_controller)
        return;

    if (m_componentCP && m_controllerCP)
    {
        LOG_DEBUG("Disconnecting connection points...");
        m_componentCP->disconnect(m_controllerCP);
        m_controllerCP->disconnect(m_componentCP);
        m_controllerCP = nullptr;
        m_componentCP  = nullptr;
        LOG_DEBUG("Connection points disconnected");
    }

    if (m_controller && m_componentHandler)
    {
        LOG_DEBUG("Removing component handler...");
        m_controller->setComponentHandler(nullptr);
    }

    if (m_isControllerSeparate && m_controller)
    {
        LOG_DEBUG("Terminating separate controller...");
        m_controller->terminate();
    }

    m_controller       = nullptr;
    m_componentHandler = nullptr;
    LOG_DEBUG("Controller cleanup complete");
}

void Vst3Plugin::CleanupComponent()
{
    LOG_DEBUG("CleanupComponent started");

    m_processor = nullptr;

    if (m_component)
    {
        LOG_DEBUG("Terminating component...");
        m_component->terminate();
        m_component = nullptr;
    }

    if (m_eventList)
    {
        m_eventList->release();
        m_eventList = nullptr;
    }

    m_hostAppController = nullptr;
    m_hostAppComponent  = nullptr;

    LOG_DEBUG("Component cleanup complete");
}

Vst3Plugin::~Vst3Plugin()
{
    std::cout << "Destroying plugin: " << m_name << std::endl;

    m_isLoaded = false;

    // Stop audio FIRST before touching the editor. Complex plugins (Kontakt,
    // Guitar Rig, etc.) share state between their audio and UI threads.
    // Tearing down the editor while the plugin is still processing causes
    // race conditions and crashes.
    CleanupProcessing();
    CleanupEditor();
    CleanupController();
    CleanupComponent();

    // Release our shared_ptr reference. The module cache still holds a
    // reference, so the DLL is NOT unloaded — intentional. See the comment
    // above s_moduleCache for the rationale.
    m_module = nullptr;

    std::cout << "Plugin destroyed: " << m_name << std::endl;
}

void Vst3Plugin::AttachEditor(void* handle)
{
    LOG_DEBUG("AttachEditor called");

    if (!m_controller)
    {
        std::cerr << "No controller available for editor" << std::endl;
        return;
    }

    if (m_plugView)
    {
        LOG_DEBUG("Cleaning up existing editor...");
        CleanupEditor();
    }

    LOG_DEBUG("Creating view...");
    IPlugView* plugView = m_controller->createView(ViewType::kEditor);
    if (!plugView)
    {
        std::cerr << "Failed to create editor view" << std::endl;
        return;
    }

    m_plugView = plugView;

#if defined(_WIN32)
    FIDString platformType = kPlatformTypeHWND;
#elif defined(__linux__)
    FIDString platformType = kPlatformTypeX11EmbedWindowID;
#elif defined(__APPLE__)
    FIDString platformType = kPlatformTypeNSView;
#else
    FIDString platformType = kPlatformTypeHWND;
#endif

    LOG_DEBUG("Checking platform type support (" << platformType << ")...");
    if (m_plugView->isPlatformTypeSupported(platformType) != kResultOk)
    {
        std::cerr << "Editor platform type not supported" << std::endl;
        m_plugView = nullptr;
        return;
    }

    LOG_DEBUG("Creating frame...");
    PlugFrame* frame = new PlugFrame(handle);
    m_plugFrame = frame;

    LOG_DEBUG("Setting frame...");
    if (m_plugView->setFrame(frame) != kResultOk)
        std::cerr << "Warning: setFrame returned error" << std::endl;

    LOG_DEBUG("Attaching view...");
    if (m_plugView->attached(handle, platformType) != kResultOk)
    {
        std::cerr << "Failed to attach editor view" << std::endl;
        m_plugView->setFrame(nullptr);
        m_plugFrame->release();
        m_plugFrame = nullptr;
        m_plugView  = nullptr;
        return;
    }

    ViewRect pluginRect;
    if (m_plugView->getSize(&pluginRect) == kResultOk)
    {
        int width  = pluginRect.getWidth();
        int height = pluginRect.getHeight();
        LOG_DEBUG("Editor size: " << width << "x" << height);

#if defined(_WIN32)
        RECT rc;
        GetWindowRect(static_cast<HWND>(handle), &rc);
        int x = rc.left;
        int y = rc.top;

        SetWindowPos(
            static_cast<HWND>(handle),
            nullptr,
            x, y,
            width + 16, height + 38,
            SWP_NOZORDER | SWP_NOACTIVATE
        );
#elif defined(__linux__)
        Display* display = XOpenDisplay(nullptr);
        if (display)
        {
            Window win = reinterpret_cast<Window>(reinterpret_cast<uintptr_t>(handle));
            XResizeWindow(display, win, (unsigned int)width, (unsigned int)height);
            XFlush(display);
            XCloseDisplay(display);
        }
#endif

        m_plugView->onSize(&pluginRect);
    }

    LOG_DEBUG("AttachEditor complete");
}

void Vst3Plugin::DetachEditor()
{
    CleanupEditor();
}

void Vst3Plugin::IdleEditor()
{
    if (!m_isLoaded)
        return;

    // Drive the plugin's IRunLoop — polls registered X11 FDs and fires any
    // timers that are due. This replaces the need for separate OS timers;
    // your existing render loop calling IdleEditor() is sufficient.
#if defined(__linux__)
    if (m_plugFrame)
        m_plugFrame->RunIdle();
#endif

    if (m_plugView)
    {
        ViewRect rect;
        if (m_plugView->getSize(&rect) == kResultOk)
            m_plugView->checkSizeConstraint(&rect);
    }

    if (m_controller && outputParamChanges.getParameterCount() > 0)
    {
        for (int32 i = 0; i < outputParamChanges.getParameterCount(); i++)
        {
            IParamValueQueue* queue = outputParamChanges.getParameterData(i);
            if (queue)
            {
                ParamValue value;
                int32 sampleOffset;
                int32 numPoints = queue->getPointCount();

                if (numPoints > 0 &&
                    queue->getPoint(numPoints - 1, sampleOffset, value) == kResultOk)
                {
                    m_controller->setParamNormalized(queue->getParameterId(), value);
                }
            }
        }
        outputParamChanges.clearQueue();
    }
}

void Vst3Plugin::PlayNote(int32_t key, int32_t velocity, int32_t sampleOffset)
{
    if (!m_isLoaded || !m_eventList)
        return;

    Event event;
    memset(&event, 0, sizeof(Event));

    event.busIndex     = 0;
    event.sampleOffset = sampleOffset;
    event.ppqPosition  = 0.0;
    event.flags        = Event::kIsLive;
    event.type         = Event::kNoteOnEvent;

    event.noteOn.channel  = 0;
    event.noteOn.pitch    = key;
    event.noteOn.tuning   = 0.0f;
    event.noteOn.velocity = velocity / 127.0f;
    event.noteOn.length   = 0;
    event.noteOn.noteId   = -1;

    m_eventList->addEvent(event);
}

void Vst3Plugin::StopNote(int32_t key, int32_t sampleOffset)
{
    if (!m_isLoaded || !m_eventList)
        return;

    Event event;
    memset(&event, 0, sizeof(Event));

    event.busIndex     = 0;
    event.sampleOffset = sampleOffset;
    event.ppqPosition  = 0.0;
    event.flags        = Event::kIsLive;
    event.type         = Event::kNoteOffEvent;

    event.noteOff.channel  = 0;
    event.noteOff.pitch    = key;
    event.noteOff.velocity = 0.0f;
    event.noteOff.noteId   = -1;
    event.noteOff.tuning   = 0.0f;

    m_eventList->addEvent(event);
}

void Vst3Plugin::StopAllNotes()
{
    if (!m_isLoaded || !m_eventList)
        return;

    for (int32_t key = 0; key < 128; key++)
    {
        Event event;
        memset(&event, 0, sizeof(Event));

        event.busIndex     = 0;
        event.sampleOffset = 0;
        event.ppqPosition  = 0.0;
        event.flags        = Event::kIsLive;
        event.type         = Event::kNoteOffEvent;

        event.noteOff.channel  = 0;
        event.noteOff.pitch    = key;
        event.noteOff.velocity = 0.0f;
        event.noteOff.noteId   = -1;
        event.noteOff.tuning   = 0.0f;

        m_eventList->addEvent(event);
    }
}

void Vst3Plugin::Process(float** inputs, float** outputs, int32_t sampleFrames, float bpm)
{
    if (!m_isLoaded)
        return;

    if (!m_processor || !m_processingActive)
    {
        for (int32_t i = 0; i < m_outputChannels; i++)
        {
            if (outputs && outputs[i])
                memset(outputs[i], 0, sampleFrames * sizeof(float));
        }
        return;
    }

    if (sampleFrames <= 0 || sampleFrames > m_maxBlockSize)
        return;

    // ------------------------------------------------------------------
    // Wire up input bus channel buffer pointers.
    //
    // m_inputBusChannelOffset[i] is the cumulative channel index into
    // the flat `inputs` pointer array at which bus i starts. This is
    // computed once in SetupProcessing so that buses with different
    // channel counts are addressed correctly (e.g. bus 0 = stereo at
    // offset 0, bus 1 = stereo at offset 2, NOT at offset 1*2 = 2 only
    // by coincidence — the old code just happened to work for equal-
    // channel buses but would silently mis-address unequal ones).
    // ------------------------------------------------------------------
    for (int32_t i = 0; i < m_numInputBuses; i++)
    {
        inputBuses[i].silenceFlags = 0;

        if (inputs && inputBuses[i].numChannels > 0)
        {
            inputBuses[i].channelBuffers32 = inputs + m_inputBusChannelOffset[i];

            bool isSilent = true;
            for (int32_t ch = 0; ch < inputBuses[i].numChannels && isSilent; ch++)
            {
                float* buffer = inputBuses[i].channelBuffers32[ch];
                if (buffer)
                {
                    int32_t checkSamples = std::min(sampleFrames, 32);
                    for (int32_t s = 0; s < checkSamples; s++)
                    {
                        if (buffer[s] != 0.0f)
                        {
                            isSilent = false;
                            break;
                        }
                    }
                }
            }

            if (isSilent)
                inputBuses[i].silenceFlags = (1ULL << inputBuses[i].numChannels) - 1;
        }
        else
        {
            inputBuses[i].channelBuffers32 = nullptr;
            inputBuses[i].silenceFlags     = (1ULL << inputBuses[i].numChannels) - 1;
        }
    }

    // ------------------------------------------------------------------
    // Wire up output bus channel buffer pointers.
    //
    // Same cumulative-offset logic as inputs above. The original code
    // used `outputs + (i * outputBuses[i].numChannels)` which is only
    // correct when every bus has the same number of channels. Using the
    // pre-computed offset handles the general case.
    // ------------------------------------------------------------------
    for (int32_t i = 0; i < m_numOutputBuses; i++)
    {
        if (outputs && outputBuses[i].numChannels > 0)
        {
            outputBuses[i].channelBuffers32 = outputs + m_outputBusChannelOffset[i];
            outputBuses[i].silenceFlags     = 0;
        }
        else
        {
            outputBuses[i].channelBuffers32 = nullptr;
            outputBuses[i].silenceFlags     = (1ULL << outputBuses[i].numChannels) - 1;
        }
    }

    processData.numSamples = sampleFrames;

    processContext.tempo = bpm > 0.0f ? bpm : 120.0;
    processContext.projectTimeSamples   += sampleFrames;
    processContext.continousTimeSamples += sampleFrames;
    processContext.systemTime = (uint64_t)(processContext.continousTimeSamples / m_sampleRate * 1e9);

    double samplesPerBeat = (60.0 / processContext.tempo) * m_sampleRate;
    processContext.projectTimeMusic = processContext.projectTimeSamples / samplesPerBeat;
    processContext.barPositionMusic = fmod(processContext.projectTimeMusic,
                                          processContext.timeSigNumerator);

    m_processor->process(processData);

    if (m_eventList)
        m_eventList->clear();

    inputParamChanges.clearQueue();
    outputParamChanges.clearQueue();
}

void Vst3Plugin::SetupProcessing(double sampleRate, int32_t maxBlockSize)
{
    LOG_DEBUG("SetupProcessing started");

    if (!m_processor || !m_component)
    {
        std::cerr << "Cannot setup processing - processor or component is null" << std::endl;
        return;
    }

    m_sampleRate   = sampleRate;
    m_maxBlockSize = maxBlockSize;

    m_numInputBuses  = m_component->getBusCount(Steinberg::Vst::kAudio, Steinberg::Vst::kInput);
    m_numOutputBuses = m_component->getBusCount(Steinberg::Vst::kAudio, Steinberg::Vst::kOutput);
    int32 numEventInputBuses = m_component->getBusCount(Steinberg::Vst::kEvent, Steinberg::Vst::kInput);

    std::cout << "Plugin has " << m_numInputBuses  << " input audio buses and "
              << m_numOutputBuses << " output audio buses" << std::endl;
    std::cout << "Plugin has " << numEventInputBuses << " event input buses" << std::endl;

    if (m_numInputBuses  > 2) m_numInputBuses  = 2;
    if (m_numOutputBuses > 2) m_numOutputBuses = 2;

    m_inputChannels  = 0;
    m_outputChannels = 0;

    // Activate input buses and record per-bus channel offsets.
    for (int32_t i = 0; i < m_numInputBuses; i++)
    {
        m_inputBusChannelOffset[i] = m_inputChannels; // offset = channels accumulated so far

        BusInfo busInfo;
        if (m_component->getBusInfo(Steinberg::Vst::kAudio, Steinberg::Vst::kInput, i, busInfo) == kResultOk)
        {
            m_component->activateBus(Steinberg::Vst::kAudio, Steinberg::Vst::kInput, i, true);
            inputBuses[i].numChannels  = busInfo.channelCount;
            inputBuses[i].silenceFlags = 0;
            m_inputChannels += busInfo.channelCount;
        }
        else
        {
            inputBuses[i].numChannels = 0;
        }
    }

    // Activate output buses and record per-bus channel offsets.
    for (int32_t i = 0; i < m_numOutputBuses; i++)
    {
        m_outputBusChannelOffset[i] = m_outputChannels; // offset = channels accumulated so far

        BusInfo busInfo;
        if (m_component->getBusInfo(Steinberg::Vst::kAudio, Steinberg::Vst::kOutput, i, busInfo) == kResultOk)
        {
            m_component->activateBus(Steinberg::Vst::kAudio, Steinberg::Vst::kOutput, i, true);
            outputBuses[i].numChannels  = busInfo.channelCount;
            outputBuses[i].silenceFlags = 0;
            m_outputChannels += busInfo.channelCount;
        }
        else
        {
            outputBuses[i].numChannels = 0;
        }
    }

    if (numEventInputBuses > 0)
        m_component->activateBus(Steinberg::Vst::kEvent, Steinberg::Vst::kInput, 0, true);

    processData.numInputs  = m_numInputBuses;
    processData.numOutputs = m_numOutputBuses;

    if (m_outputChannels == 0)
    {
        std::cerr << "Error: Plugin has no output channels" << std::endl;
        return;
    }

    Steinberg::Vst::ProcessSetup setup;
    setup.processMode        = Steinberg::Vst::kRealtime;
    setup.symbolicSampleSize = Steinberg::Vst::kSample32;
    setup.maxSamplesPerBlock = maxBlockSize;
    setup.sampleRate         = sampleRate;

    tresult setupResult = m_processor->setupProcessing(setup);
    if (setupResult != kResultOk)
    {
        std::cerr << "setupProcessing failed with result: " << setupResult << std::endl;
        return;
    }

    tresult activeResult = m_component->setActive(true);
    if (activeResult != kResultOk)
    {
        std::cerr << "Failed to activate component, result: " << activeResult << std::endl;
        return;
    }
    m_componentActive = true;

    tresult processingResult = m_processor->setProcessing(true);
    if (processingResult != kResultOk && processingResult != kNotImplemented)
    {
        std::cerr << "setProcessing(true) failed with result: " << processingResult << std::endl;
        // Some plugins return kNotImplemented here — that is acceptable and
        // does not indicate a failure. Any other non-OK result is a real error,
        // but we continue anyway since some plugins misbehave here yet still
        // produce audio correctly.
    }
    m_processingActive = true;

    std::cout << "Processing setup complete (Input: " << m_inputChannels
              << " channels in " << m_numInputBuses << " bus(es), Output: " << m_outputChannels
              << " channels in " << m_numOutputBuses << " bus(es))" << std::endl;

    LOG_DEBUG("Bus channel offsets:");
    for (int32_t i = 0; i < m_numInputBuses; i++)
        LOG_DEBUG("  Input bus " << i << ": offset=" << m_inputBusChannelOffset[i]
                                 << " channels=" << inputBuses[i].numChannels);
    for (int32_t i = 0; i < m_numOutputBuses; i++)
        LOG_DEBUG("  Output bus " << i << ": offset=" << m_outputBusChannelOffset[i]
                                  << " channels=" << outputBuses[i].numChannels);
}

void Vst3Plugin::Serialize(const char* filePath)
{
    if (!m_component)
    {
        std::cerr << "Cannot serialize - component is null" << std::endl;
        return;
    }

    MemoryStream stream;
    if (m_component->getState(&stream) != kResultOk)
    {
        std::cerr << "Failed to get component state" << std::endl;
        return;
    }

    std::ofstream file(filePath, std::ios::binary);
    if (!file)
    {
        std::cerr << "Failed to open file for writing: " << filePath << std::endl;
        return;
    }

    int64 streamSize = 0;
    stream.seek(0, IBStream::kIBSeekEnd, &streamSize);
    stream.seek(0, IBStream::kIBSeekSet, nullptr);

    std::vector<char> buffer(streamSize);
    int32 bytesRead = 0;
    stream.read(buffer.data(), streamSize, &bytesRead);

    file.write(buffer.data(), bytesRead);
    std::cout << "Saved state to " << filePath << " (" << bytesRead << " bytes)" << std::endl;
}

void Vst3Plugin::Deserialize(const char* filePath)
{
    if (!m_component)
    {
        std::cerr << "Cannot deserialize - component is null" << std::endl;
        return;
    }

    std::ifstream file(filePath, std::ios::binary);
    if (!file)
    {
        std::cerr << "Failed to open file for reading: " << filePath << std::endl;
        return;
    }

    std::vector<char> buffer((std::istreambuf_iterator<char>(file)),
                              std::istreambuf_iterator<char>());

    if (buffer.empty())
    {
        std::cerr << "Empty state file" << std::endl;
        return;
    }

    MemoryStream stream(buffer.data(), buffer.size());

    if (m_component->setState(&stream) != kResultOk)
    {
        std::cerr << "Failed to set component state" << std::endl;
        return;
    }

    if (m_controller && m_isControllerSeparate)
    {
        stream.seek(0, IBStream::kIBSeekSet, nullptr);
        m_controller->setComponentState(&stream);
    }

    std::cout << "Loaded state from " << filePath << " (" << buffer.size() << " bytes)" << std::endl;
}

Plugin* PluginLoader::Load(const char* path, float sampleRate, int32_t blockSize)
{
    LOG_DEBUG("PluginLoader::Load called");
    Vst3Plugin* plugin = new Vst3Plugin(path, sampleRate, blockSize);
    if (!plugin->IsLoaded())
    {
        LOG_DEBUG("Plugin failed to load, deleting...");
        delete plugin;
        return nullptr;
    }
    LOG_DEBUG("Plugin loaded successfully");
    return plugin;
}

} // namespace Uvi