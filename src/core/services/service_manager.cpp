#include "services/service_manager.hpp"

ServiceManager::ServiceManager(std::array<u32, 16>& regs, Memory& mem) : regs(regs), mem(mem), apt(mem) {}

void ServiceManager::reset() {
	apt.reset();
}

// Match IPC messages to a "srv:" command based on their header
namespace Commands {
	enum : u32 {
		RegisterClient = 0x00010002,
		EnableNotification = 0x00020000,
		RegisterService = 0x00030100,
		UnregisterService = 0x000400C0,
		GetServiceHandle = 0x00050100,
		RegisterPort = 0x000600C2,
		UnregisterPort = 0x000700C0,
		GetPort = 0x00080100,
		Subscribe = 0x00090040,
		Unsubscribe = 0x000A0040,
		ReceiveNotification = 0x000B0000,
		PublishToSubscriber = 0x000C0080,
		PublishAndGetSubscriber = 0x000D0040,
		IsServiceRegistered = 0x000E00C0
	};
}

namespace Result {
	enum : u32 {
		Success = 0
	};
}

// Handle an IPC message issued using the SendSyncRequest SVC
// The parameters are stored in thread-local storage in this format: https://www.3dbrew.org/wiki/IPC#Message_Structure
// messagePointer: The base pointer for the IPC message
void ServiceManager::handleSyncRequest(u32 messagePointer) {
	const u32 header = mem.read32(messagePointer);

	switch (header) {
		case Commands::RegisterClient: registerClient(messagePointer); break;
		case Commands::GetServiceHandle: getServiceHandle(messagePointer); break;
		default: Helpers::panic("Unknown \"srv:\" command: %08X", header);
	}
}

// https://www.3dbrew.org/wiki/SRV:RegisterClient
void ServiceManager::registerClient(u32 messagePointer) {
	printf("srv: registerClient (Stubbed)\n");
	mem.write32(messagePointer + 4, Result::Success);
}

// https://www.3dbrew.org/wiki/SRV:GetServiceHandle
void ServiceManager::getServiceHandle(u32 messagePointer) {
	u32 nameLength = mem.read32(messagePointer + 12);
	u32 flags = mem.read32(messagePointer + 16);
	u32 handle = 0;

	std::string service = mem.readString(messagePointer + 4, 8);
	printf("srv: getServiceHandle (Service: %s, nameLength: %d, flags: %d)\n", service.c_str(), nameLength, flags);

	if (service == "APT:S") {
		handle = KernelHandles::APT;
	} else {
		Helpers::panic("srv: GetServiceHandle with unknown service %s", service.c_str());
	}

	mem.write32(messagePointer + 4, Result::Success);
	mem.write32(messagePointer + 12, handle);
}

void ServiceManager::sendCommandToService(u32 messagePointer, Handle handle) {
	switch (handle) {
		case KernelHandles::APT: apt.handleSyncRequest(messagePointer); break;
		default: Helpers::panic("Sent IPC message to unknown service %08X\n", handle);
	}
}