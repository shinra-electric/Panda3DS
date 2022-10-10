#pragma once
#include <array>
#include "helpers.hpp"
#include "logger.hpp"
#include "memory.hpp"
#include "services/apt.hpp"
#include "services/cfg.hpp"
#include "services/dsp.hpp"
#include "services/hid.hpp"
#include "services/fs.hpp"
#include "services/gsp_gpu.hpp"
#include "services/gsp_lcd.hpp"
#include "services/ndm.hpp"

class Kernel;

class ServiceManager {
	std::array<u32, 16>& regs;
	Memory& mem;
	MAKE_LOG_FUNCTION(log, srvLogger)

	APTService apt;
	CFGService cfg;
	DSPService dsp;
	HIDService hid;
	FSService fs;
	GPUService gsp_gpu;
	LCDService gsp_lcd;
	NDMService ndm;

	// "srv:" commands
	void enableNotification(u32 messagePointer);
	void getServiceHandle(u32 messagePointer);
	void receiveNotification(u32 messagePointer);
	void registerClient(u32 messagePointer);

public:
	ServiceManager(std::array<u32, 16>& regs, Memory& mem, GPU& gpu, u32& currentPID, Kernel& kernel);
	void reset();
	void handleSyncRequest(u32 messagePointer);

	// Forward a SendSyncRequest IPC message to the service with the respective handle
	void sendCommandToService(u32 messagePointer, Handle handle);

	// Wrappers for communicating with certain services
	void requestGPUInterrupt(GPUInterrupt type) { gsp_gpu.requestInterrupt(type); }
	void setGSPSharedMem(u8* ptr) { gsp_gpu.setSharedMem(ptr); }
	void setHIDSharedMem(u8* ptr) { hid.setSharedMem(ptr); }
};