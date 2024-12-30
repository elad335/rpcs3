#pragma once

#include <map>
#include <vector>

// Node location
using cfg_location = std::vector<const char*>;

enum class emu_settings_type
{
	// Core
	PPUDecoder,
	SPUDecoder,
	HookStaticFuncs,
	ThreadSchedulerMode,
	SPULoopDetection,
	PreferredSPUThreads,
	PPUDebug,
	SPUDebug,
	MFCDebug,
	MaxLLVMThreads,
	LLVMPrecompilation,
	EnableTSX,
	AccurateSpuDMA,
	AccurateClineStores,
	AccurateRSXAccess,
	FIFOAccuracy,
	XFloatAccuracy,
	AccuratePPU128Loop,
	MFCCommandsShuffling,
	NumPPUThreads,
	SetDAZandFTZ,
	SPUBlockSize,
	SPUCache,
	DebugConsoleMode,
	SilenceAllLogs,
	SuspendEmulationSavestateMode,
	CompatibleEmulationSavestateMode,
	StartSavestatePaused,
	MaxSPURSThreads,
	SleepTimersAccuracy,
	ClocksScale,
	PerformanceReport,
	FullWidthAVX512,
	PPUNJFixup,
	AccurateDFMA,
	AccuratePPUSAT,
	AccuratePPUNJ,
	FixupPPUVNAN,
	AccuratePPUVNAN,
	AccuratePPUFPCC,
	MaxPreemptCount,
	SPUProfiler,

	// Graphics
	Renderer,
	Resolution,
	AspectRatio,
	FrameLimit,
	MSAA,
	LogShaderPrograms,
	WriteDepthBuffer,
	WriteColorBuffers,
	ReadColorBuffers,
	ReadDepthBuffer,
	HandleRSXTiledMemory,
	VSync,
	DebugOutput,
	DebugOverlay,
	RenderdocCompatibility,
	GPUTextureScaling,
	StretchToDisplayArea,
	VulkanAdapter,
	ForceHighpZ,
	StrictRenderingMode,
	DisableVertexCache,
	DisableOcclusionQueries,
	DisableVideoOutput,
	DisableFIFOReordering,
	StrictTextureFlushing,
	ShaderPrecisionQuality,
	StereoRenderMode,
	AnisotropicFilterOverride,
	TextureLodBias,
	ResolutionScale,
	MinimumScalableDimension,
	FsrSharpeningStrength,
	ExclusiveFullscreenMode,
	ForceCPUBlitEmulation,
	DisableOnDiskShaderCache,
	DisableVulkanMemAllocator,
	ShaderMode,
	ShaderCompilerNumThreads,
	MultithreadedRSX,
	VBlankRate,
	VBlankNTSCFixup,
	RelaxedZCULL,
	PreciseZCULL,
	DriverWakeUpDelay,
	VulkanAsyncTextureUploads,
	VulkanAsyncSchedulerDriver,
	AllowHostGPULabels,
	DisableMSLFastMath,
	OutputScalingMode,
	ForceHwMSAAResolve,
	DisableAsyncHostMM,

	// Performance Overlay
	PerfOverlayEnabled,
	PerfOverlayFramerateGraphEnabled,
	PerfOverlayFrametimeGraphEnabled,
	PerfOverlayFramerateDatapoints,
	PerfOverlayFrametimeDatapoints,
	PerfOverlayDetailLevel,
	PerfOverlayFramerateDetailLevel,
	PerfOverlayFrametimeDetailLevel,
	PerfOverlayPosition,
	PerfOverlayUpdateInterval,
	PerfOverlayFontSize,
	PerfOverlayOpacity,
	PerfOverlayMarginX,
	PerfOverlayMarginY,
	PerfOverlayCenterX,
	PerfOverlayCenterY,

	// Shader Loading Dialog
	ShaderLoadBgEnabled,
	ShaderLoadBgDarkening,
	ShaderLoadBgBlur,

	// Audio
	AudioRenderer,
	DumpToFile,
	ConvertTo16Bit,
	AudioFormat,
	AudioFormats,
	AudioProvider,
	AudioAvport,
	AudioDevice,
	AudioChannelLayout,
	MasterVolume,
	EnableBuffering,
	AudioBufferDuration,
	EnableTimeStretching,
	TimeStretchingThreshold,
	MicrophoneType,
	MicrophoneDevices,
	MusicHandler,

	// Input / Output
	BackgroundInput,
	ShowMoveCursor,
	LockOvlIptToP1,
	PadHandlerMode,
	PadConnection,
	KeyboardHandler,
	MouseHandler,
	Camera,
	CameraType,
	CameraFlip,
	CameraID,
	Move,
	Buzz,
	Turntable,
	GHLtar,
	MidiDevices,
	SDLMappings,
	IoDebugOverlay,

	// Misc
	ExitRPCS3OnFinish,
	StartOnBoot,
	PauseOnFocusLoss,
	StartGameFullscreen,
	PreventDisplaySleep,
	ShowTrophyPopups,
	ShowRpcnPopups,
	UseNativeInterface,
	ShowShaderCompilationHint,
	ShowPPUCompilationHint,
	ShowPressureIntensityToggleHint,
	ShowAnalogLimiterToggleHint,
	ShowMouseAndKeyboardToggleHint,
	ShowAutosaveAutoloadHint,
	WindowTitleFormat,
	PauseDuringHomeMenu,

	// Network
	InternetStatus,
	DNSAddress,
	IpSwapList,
	PSNStatus,
	BindAddress,
	EnableUpnp,
	PSNCountry,

	// System
	LicenseArea,
	Language,
	KeyboardType,
	EnterButtonAssignment,
	EnableHostRoot,
	EmptyHdd0Tmp,
	LimitCacheSize,
	MaximumCacheSize,
	ConsoleTimeOffset,
};

/** A helper map that keeps track of where a given setting type is located*/
inline static const std::map<emu_settings_type, cfg_location> settings_location =
{
	// Core Tab
	{ emu_settings_type::PPUDecoder,               { "Core", "PPU Decoder"}},
	{ emu_settings_type::SPUDecoder,               { "Core", "SPU Decoder"}},
	{ emu_settings_type::HookStaticFuncs,          { "Core", "Hook static functions"}},
	{ emu_settings_type::ThreadSchedulerMode,      { "Core", "Thread Scheduler Mode"}},
	{ emu_settings_type::SPULoopDetection,         { "Core", "SPU loop detection"}},
	{ emu_settings_type::PreferredSPUThreads,      { "Core", "Preferred SPU Threads"}},
	{ emu_settings_type::PPUDebug,                 { "Core", "PPU Debug"}},
	{ emu_settings_type::SPUDebug,                 { "Core", "SPU Debug"}},
	{ emu_settings_type::MFCDebug,                 { "Core", "MFC Debug"}},
	{ emu_settings_type::MaxLLVMThreads,           { "Core", "Max LLVM Compile Threads"}},
	{ emu_settings_type::LLVMPrecompilation,       { "Core", "LLVM Precompilation"}},
	{ emu_settings_type::EnableTSX,                { "Core", "Enable TSX"}},
	{ emu_settings_type::AccurateSpuDMA,           { "Core", "Accurate SPU DMA"}},
	{ emu_settings_type::AccurateClineStores,      { "Core", "Accurate Cache Line Stores"}},
	{ emu_settings_type::AccurateRSXAccess,        { "Core", "Accurate RSX reservation access"}},
	{ emu_settings_type::FIFOAccuracy,             { "Core", "RSX FIFO Accuracy"}},
	{ emu_settings_type::XFloatAccuracy,           { "Core", "XFloat Accuracy"}},
	{ emu_settings_type::MFCCommandsShuffling,     { "Core", "MFC Commands Shuffling Limit"}},
	{ emu_settings_type::SetDAZandFTZ,             { "Core", "Set DAZ and FTZ"}},
	{ emu_settings_type::SPUBlockSize,             { "Core", "SPU Block Size"}},
	{ emu_settings_type::SPUCache,                 { "Core", "SPU Cache"}},
	{ emu_settings_type::DebugConsoleMode,         { "Core", "Debug Console Mode"}},
	{ emu_settings_type::MaxSPURSThreads,          { "Core", "Max SPURS Threads"}},
	{ emu_settings_type::SleepTimersAccuracy,      { "Core", "Sleep Timers Accuracy 2"}},
	{ emu_settings_type::ClocksScale,              { "Core", "Clocks scale"}},
	{ emu_settings_type::AccuratePPU128Loop,       { "Core", "Accurate PPU 128-byte Reservation Op Max Length"}},
	{ emu_settings_type::PerformanceReport,        { "Core", "Enable Performance Report"}},
	{ emu_settings_type::FullWidthAVX512,          { "Core", "Full Width AVX-512"}},
	{ emu_settings_type::NumPPUThreads,            { "Core", "PPU Threads"}},
	{ emu_settings_type::PPUNJFixup,               { "Core", "PPU LLVM Java Mode Handling"}},
	{ emu_settings_type::AccurateDFMA,             { "Core", "Use Accurate DFMA"}},
	{ emu_settings_type::AccuratePPUSAT,           { "Core", "PPU Set Saturation Bit"}},
	{ emu_settings_type::AccuratePPUNJ,            { "Core", "PPU Accurate Non-Java Mode"}},
	{ emu_settings_type::FixupPPUVNAN,             { "Core", "PPU Fixup Vector NaN Values"}},
	{ emu_settings_type::AccuratePPUVNAN,          { "Core", "PPU Accurate Vector NaN Values"}},
	{ emu_settings_type::AccuratePPUFPCC,          { "Core", "PPU Set FPCC Bits"}},
	{ emu_settings_type::MaxPreemptCount,          { "Core", "Max CPU Preempt Count"}},
	{ emu_settings_type::SPUProfiler,              { "Core", "SPU Profiler"}},

	// Graphics Tab
	{ emu_settings_type::Renderer,                   { "Video", "Renderer"}},
	{ emu_settings_type::Resolution,                 { "Video", "Resolution"}},
	{ emu_settings_type::AspectRatio,                { "Video", "Aspect ratio"}},
	{ emu_settings_type::FrameLimit,                 { "Video", "Frame limit"}},
	{ emu_settings_type::MSAA,                       { "Video", "MSAA"}},
	{ emu_settings_type::LogShaderPrograms,          { "Video", "Log shader programs"}},
	{ emu_settings_type::WriteDepthBuffer,           { "Video", "Write Depth Buffer"}},
	{ emu_settings_type::WriteColorBuffers,          { "Video", "Write Color Buffers"}},
	{ emu_settings_type::ReadColorBuffers,           { "Video", "Read Color Buffers"}},
	{ emu_settings_type::ReadDepthBuffer,            { "Video", "Read Depth Buffer"}},
	{ emu_settings_type::HandleRSXTiledMemory,       { "Video", "Handle RSX Memory Tiling"}},
	{ emu_settings_type::VSync,                      { "Video", "VSync"}},
	{ emu_settings_type::DebugOutput,                { "Video", "Debug output"}},
	{ emu_settings_type::DebugOverlay,               { "Video", "Debug overlay"}},
	{ emu_settings_type::RenderdocCompatibility,     { "Video", "Renderdoc Compatibility Mode"}},
	{ emu_settings_type::GPUTextureScaling,          { "Video", "Use GPU texture scaling"}},
	{ emu_settings_type::StretchToDisplayArea,       { "Video", "Stretch To Display Area"}},
	{ emu_settings_type::ForceHighpZ,                { "Video", "Force High Precision Z buffer"}},
	{ emu_settings_type::StrictRenderingMode,        { "Video", "Strict Rendering Mode"}},
	{ emu_settings_type::DisableVertexCache,         { "Video", "Disable Vertex Cache"}},
	{ emu_settings_type::DisableOcclusionQueries,    { "Video", "Disable ZCull Occlusion Queries"}},
	{ emu_settings_type::DisableVideoOutput,         { "Video", "Disable Video Output"}},
	{ emu_settings_type::DisableFIFOReordering,      { "Video", "Disable FIFO Reordering"}},
	{ emu_settings_type::StereoRenderMode,           { "Video", "3D Display Mode"}},
	{ emu_settings_type::StrictTextureFlushing,      { "Video", "Strict Texture Flushing"}},
	{ emu_settings_type::ForceCPUBlitEmulation,      { "Video", "Force CPU Blit"}},
	{ emu_settings_type::DisableOnDiskShaderCache,   { "Video", "Disable On-Disk Shader Cache"}},
	{ emu_settings_type::DisableVulkanMemAllocator,  { "Video", "Disable Vulkan Memory Allocator"}},
	{ emu_settings_type::ShaderMode,                 { "Video", "Shader Mode"}},
	{ emu_settings_type::ShaderCompilerNumThreads,   { "Video", "Shader Compiler Threads"}},
	{ emu_settings_type::ShaderPrecisionQuality,     { "Video", "Shader Precision"}},
	{ emu_settings_type::MultithreadedRSX,           { "Video", "Multithreaded RSX"}},
	{ emu_settings_type::RelaxedZCULL,               { "Video", "Relaxed ZCULL Sync"}},
	{ emu_settings_type::PreciseZCULL,               { "Video", "Accurate ZCULL stats"}},
	{ emu_settings_type::AnisotropicFilterOverride,  { "Video", "Anisotropic Filter Override"}},
	{ emu_settings_type::TextureLodBias,             { "Video", "Texture LOD Bias Addend"}},
	{ emu_settings_type::ResolutionScale,            { "Video", "Resolution Scale"}},
	{ emu_settings_type::MinimumScalableDimension,   { "Video", "Minimum Scalable Dimension"}},
	{ emu_settings_type::VulkanAdapter,              { "Video", "Vulkan", "Adapter"}},
	{ emu_settings_type::VBlankRate,                 { "Video", "Vblank Rate"}},
	{ emu_settings_type::VBlankNTSCFixup,            { "Video", "Vblank NTSC Fixup"}},
	{ emu_settings_type::DriverWakeUpDelay,          { "Video", "Driver Wake-Up Delay"}},
	{ emu_settings_type::AllowHostGPULabels,         { "Video", "Allow Host GPU Labels"}},
	{ emu_settings_type::DisableMSLFastMath,         { "Video", "Disable MSL Fast Math"}},
	{ emu_settings_type::OutputScalingMode,          { "Video", "Output Scaling Mode"}},
	{ emu_settings_type::ForceHwMSAAResolve,         { "Video", "Force Hardware MSAA Resolve"}},
	{ emu_settings_type::DisableAsyncHostMM,         { "Video", "Disable Asynchronous Memory Manager"}},

	// Vulkan
	{ emu_settings_type::VulkanAsyncTextureUploads,           { "Video", "Vulkan", "Asynchronous Texture Streaming 2"}},
	{ emu_settings_type::VulkanAsyncSchedulerDriver,          { "Video", "Vulkan", "Asynchronous Queue Scheduler"}},
	{ emu_settings_type::FsrSharpeningStrength,               { "Video", "Vulkan", "FidelityFX CAS Sharpening Intensity"}},
	{ emu_settings_type::ExclusiveFullscreenMode,             { "Video", "Vulkan", "Exclusive Fullscreen Mode"}},

	// Performance Overlay
	{ emu_settings_type::PerfOverlayEnabled,               { "Video", "Performance Overlay", "Enabled" } },
	{ emu_settings_type::PerfOverlayFramerateGraphEnabled, { "Video", "Performance Overlay", "Enable Framerate Graph" } },
	{ emu_settings_type::PerfOverlayFrametimeGraphEnabled, { "Video", "Performance Overlay", "Enable Frametime Graph" } },
	{ emu_settings_type::PerfOverlayFramerateDatapoints,   { "Video", "Performance Overlay", "Framerate datapoints" } },
	{ emu_settings_type::PerfOverlayFrametimeDatapoints,   { "Video", "Performance Overlay", "Frametime datapoints" } },
	{ emu_settings_type::PerfOverlayDetailLevel,           { "Video", "Performance Overlay", "Detail level" } },
	{ emu_settings_type::PerfOverlayFramerateDetailLevel,  { "Video", "Performance Overlay", "Framerate graph detail level" } },
	{ emu_settings_type::PerfOverlayFrametimeDetailLevel,  { "Video", "Performance Overlay", "Frametime graph detail level" } },
	{ emu_settings_type::PerfOverlayPosition,              { "Video", "Performance Overlay", "Position" } },
	{ emu_settings_type::PerfOverlayUpdateInterval,        { "Video", "Performance Overlay", "Metrics update interval (ms)" } },
	{ emu_settings_type::PerfOverlayFontSize,              { "Video", "Performance Overlay", "Font size (px)" } },
	{ emu_settings_type::PerfOverlayOpacity,               { "Video", "Performance Overlay", "Opacity (%)" } },
	{ emu_settings_type::PerfOverlayMarginX,               { "Video", "Performance Overlay", "Horizontal Margin (px)" } },
	{ emu_settings_type::PerfOverlayMarginY,               { "Video", "Performance Overlay", "Vertical Margin (px)" } },
	{ emu_settings_type::PerfOverlayCenterX,               { "Video", "Performance Overlay", "Center Horizontally" } },
	{ emu_settings_type::PerfOverlayCenterY,               { "Video", "Performance Overlay", "Center Vertically" } },

	// Shader Loading Dialog
	{ emu_settings_type::ShaderLoadBgEnabled,      { "Video", "Shader Loading Dialog", "Allow custom background" } },
	{ emu_settings_type::ShaderLoadBgDarkening,    { "Video", "Shader Loading Dialog", "Darkening effect strength" } },
	{ emu_settings_type::ShaderLoadBgBlur,         { "Video", "Shader Loading Dialog", "Blur effect strength" } },

	// Audio
	{ emu_settings_type::AudioRenderer,           { "Audio", "Renderer"}},
	{ emu_settings_type::DumpToFile,              { "Audio", "Dump to file"}},
	{ emu_settings_type::ConvertTo16Bit,          { "Audio", "Convert to 16 bit"}},
	{ emu_settings_type::AudioFormat,             { "Audio", "Audio Format"}},
	{ emu_settings_type::AudioFormats,            { "Audio", "Audio Formats"}},
	{ emu_settings_type::AudioProvider,           { "Audio", "Audio Provider"}},
	{ emu_settings_type::AudioAvport,             { "Audio", "RSXAudio Avport"}},
	{ emu_settings_type::AudioDevice,             { "Audio", "Audio Device"}},
	{ emu_settings_type::AudioChannelLayout,      { "Audio", "Audio Channel Layout"}},
	{ emu_settings_type::MasterVolume,            { "Audio", "Master Volume"}},
	{ emu_settings_type::EnableBuffering,         { "Audio", "Enable Buffering"}},
	{ emu_settings_type::AudioBufferDuration,     { "Audio", "Desired Audio Buffer Duration"}},
	{ emu_settings_type::EnableTimeStretching,    { "Audio", "Enable Time Stretching"}},
	{ emu_settings_type::TimeStretchingThreshold, { "Audio", "Time Stretching Threshold"}},
	{ emu_settings_type::MicrophoneType,          { "Audio", "Microphone Type" }},
	{ emu_settings_type::MicrophoneDevices,       { "Audio", "Microphone Devices" }},
	{ emu_settings_type::MusicHandler,            { "Audio", "Music Handler"}},

	// Input / Output
	{ emu_settings_type::BackgroundInput, { "Input/Output", "Background input enabled"}},
	{ emu_settings_type::ShowMoveCursor,  { "Input/Output", "Show move cursor"}},
	{ emu_settings_type::LockOvlIptToP1,  { "Input/Output", "Lock overlay input to player one"}},
	{ emu_settings_type::PadHandlerMode,  { "Input/Output", "Pad handler mode"}},
	{ emu_settings_type::PadConnection,   { "Input/Output", "Keep pads connected" }},
	{ emu_settings_type::KeyboardHandler, { "Input/Output", "Keyboard"}},
	{ emu_settings_type::MouseHandler,    { "Input/Output", "Mouse"}},
	{ emu_settings_type::Camera,          { "Input/Output", "Camera"}},
	{ emu_settings_type::CameraType,      { "Input/Output", "Camera type"}},
	{ emu_settings_type::CameraFlip,      { "Input/Output", "Camera flip"}},
	{ emu_settings_type::CameraID,        { "Input/Output", "Camera ID"}},
	{ emu_settings_type::Move,            { "Input/Output", "Move" }},
	{ emu_settings_type::Buzz,            { "Input/Output", "Buzz emulated controller" }},
	{ emu_settings_type::Turntable,       { "Input/Output", "Turntable emulated controller" }},
	{ emu_settings_type::GHLtar,          { "Input/Output", "GHLtar emulated controller" }},
	{ emu_settings_type::MidiDevices,     { "Input/Output", "Emulated Midi devices" }},
	{ emu_settings_type::SDLMappings,     { "Input/Output", "Load SDL GameController Mappings" }},
	{ emu_settings_type::IoDebugOverlay,  { "Input/Output", "IO Debug overlay" }},

	// Misc
	{ emu_settings_type::ExitRPCS3OnFinish,               { "Miscellaneous", "Exit RPCS3 when process finishes" }},
	{ emu_settings_type::StartOnBoot,                     { "Miscellaneous", "Automatically start games after boot" }},
	{ emu_settings_type::PauseOnFocusLoss,                { "Miscellaneous", "Pause emulation on RPCS3 focus loss" }},
	{ emu_settings_type::StartGameFullscreen,             { "Miscellaneous", "Start games in fullscreen mode"}},
	{ emu_settings_type::PreventDisplaySleep,             { "Miscellaneous", "Prevent display sleep while running games"}},
	{ emu_settings_type::ShowTrophyPopups,                { "Miscellaneous", "Show trophy popups"}},
	{ emu_settings_type::ShowRpcnPopups,                  { "Miscellaneous", "Show RPCN popups"}},
	{ emu_settings_type::UseNativeInterface,              { "Miscellaneous", "Use native user interface"}},
	{ emu_settings_type::ShowShaderCompilationHint,       { "Miscellaneous", "Show shader compilation hint"}},
	{ emu_settings_type::ShowPPUCompilationHint,          { "Miscellaneous", "Show PPU compilation hint"}},
	{ emu_settings_type::ShowPressureIntensityToggleHint, { "Miscellaneous", "Show pressure intensity toggle hint"}},
	{ emu_settings_type::ShowAnalogLimiterToggleHint,     { "Miscellaneous", "Show analog limiter toggle hint"}},
	{ emu_settings_type::ShowMouseAndKeyboardToggleHint,  { "Miscellaneous", "Show mouse and keyboard toggle hint"}},
	{ emu_settings_type::SilenceAllLogs,                  { "Miscellaneous", "Silence All Logs" }},
	{ emu_settings_type::WindowTitleFormat,               { "Miscellaneous", "Window Title Format" }},
	{ emu_settings_type::PauseDuringHomeMenu,             { "Miscellaneous", "Pause Emulation During Home Menu" }},
	{ emu_settings_type::ShowAutosaveAutoloadHint,        { "Miscellaneous", "Show autosave/autoload hint" }},

	// Networking
	{ emu_settings_type::InternetStatus, { "Net", "Internet enabled"}},
	{ emu_settings_type::DNSAddress,     { "Net", "DNS address"}},
	{ emu_settings_type::IpSwapList,     { "Net", "IP swap list"}},
	{ emu_settings_type::PSNStatus,      { "Net", "PSN status"}},
	{ emu_settings_type::BindAddress,    { "Net", "Bind address"}},
	{ emu_settings_type::EnableUpnp,     { "Net", "UPNP Enabled"}},
	{ emu_settings_type::PSNCountry,     { "Net", "PSN Country"}},

	// System
	{ emu_settings_type::LicenseArea,           { "System", "License Area"}},
	{ emu_settings_type::Language,              { "System", "Language"}},
	{ emu_settings_type::KeyboardType,          { "System", "Keyboard Type"} },
	{ emu_settings_type::EnterButtonAssignment, { "System", "Enter button assignment"}},
	{ emu_settings_type::EnableHostRoot,        { "VFS", "Enable /host_root/"}},
	{ emu_settings_type::EmptyHdd0Tmp,          { "VFS", "Empty /dev_hdd0/tmp/"}},
	{ emu_settings_type::LimitCacheSize,        { "VFS", "Limit disk cache size"}},
	{ emu_settings_type::MaximumCacheSize,      { "VFS", "Disk cache maximum size (MB)"}},
	{ emu_settings_type::ConsoleTimeOffset,     { "System", "Console time offset (s)"}},

	// Savestates
	{ emu_settings_type::SuspendEmulationSavestateMode,       { "Savestate", "Suspend Emulation Savestate Mode" }},
	{ emu_settings_type::CompatibleEmulationSavestateMode,    { "Savestate", "Compatible Savestate Mode" }},
	{ emu_settings_type::StartSavestatePaused,                { "Savestate", "Start Paused" }},
};
