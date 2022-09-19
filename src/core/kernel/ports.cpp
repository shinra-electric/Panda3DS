#include "kernel.hpp"
#include <cstring>

Handle Kernel::makePort(const char* name) {
	Handle ret = makeObject(KernelObjectType::Port);
	portHandles.push_back(ret); // Push the port handle to our cache of port handles

	objects[ret].data = new PortData();
	const auto data = static_cast<PortData*>(objects[ret].data);
	std::strncpy(data->name, name, PortData::maxNameLen);

	// If the name is empty (ie the first char is the null terminator) then the port is private
	data->isPublic = name[0] != '\0';

	// printf("Created %s port \"%s\" with handle %d\n", data->isPublic ? "public" : "private", data->name, ret);
	return ret;
}

Handle Kernel::makeSession(Handle portHandle) {
	const auto port = getObject(portHandle, KernelObjectType::Port);
	if (port == nullptr) [[unlikely]] {
		Helpers::panic("Trying to make session for non-existent port");
	}

	// Allocate data for session
	const Handle ret = makeObject(KernelObjectType::Session);
	objects[ret].data = new SessionData();
	auto sessionData = static_cast<SessionData*>(objects[ret].data);

	// Link session with its parent port
	sessionData->portHandle = portHandle;
	return ret;
}

// Get the handle of a port based on its name
// If there's no such port, return nullopt
std::optional<Handle> Kernel::getPortHandle(const char* name) {
	for (auto handle : portHandles) {
		const auto data = static_cast<PortData*>(objects[handle].data);
		if (std::strncmp(name, data->name, PortData::maxNameLen) == 0) {
			return handle;
		}
	}

	return std::nullopt;
}

// Result ConnectToPort(Handle* out, const char* portName)
void Kernel::connectToPort() {
	const u32 handlePointer = regs[0];
	// Read up to max + 1 characters to see if the name is too long
	std::string port = mem.readString(regs[1], PortData::maxNameLen + 1);

	if (port.size() > PortData::maxNameLen) {
		Helpers::panic("ConnectToPort: Port name too long\n");
		regs[0] = SVCResult::PortNameTooLong;
		return;
	}

	// Try getting a handle to the port
	std::optional<Handle> optionalHandle = getPortHandle(port.c_str());
	if (!optionalHandle.has_value()) [[unlikely]] {
		Helpers::panic("ConnectToPort: Port doesn't exist\n");
		regs[0] = SVCResult::ObjectNotFound;
		return;
	}

	Handle portHandle = optionalHandle.value();
	printf("ConnectToPort(handle pointer = %08X, port = \"%s\")\n", handlePointer, port.c_str());

	const auto portData = static_cast<PortData*>(objects[portHandle].data);
	if (!portData->isPublic) {
		Helpers::panic("ConnectToPort: Attempted to connect to private port");
	}

	// TODO: Actually create session
	Handle sessionHandle = makeSession(portHandle);

	regs[0] = SVCResult::Success;
	regs[1] = sessionHandle;
}

// Result SendSyncRequest(Handle session)
// Send an IPC message to a port (typically "srv:") or a service
void Kernel::sendSyncRequest() {
	const auto handle = regs[0];
	u32 messagePointer = getTLSPointer() + 0x80; // The message is stored starting at TLS+0x80

	printf("SendSyncRequest(session handle = %X)\n", handle);

	// The sync request is being sent at a service rather than whatever port, so have the service manager intercept it
	if (KernelHandles::isServiceHandle(handle)) {
		serviceManager.sendCommandToService(messagePointer, handle);
		regs[0] = SVCResult::Success;
		return;
	}

	const auto session = getObject(handle, KernelObjectType::Session);
	if (session == nullptr) [[unlikely]] {
		Helpers::panic("SendSyncRequest: Invalid session handle");
		regs[0] = SVCResult::BadHandle;
		return;
	}

	const auto sessionData = static_cast<SessionData*>(session->data);
	const Handle portHandle = sessionData->portHandle;

	if (portHandle == srvHandle) { // Special-case SendSyncRequest targetting the "srv: port"
		serviceManager.handleSyncRequest(messagePointer);
	} else {
		const auto portData = static_cast<PortData*>(objects[portHandle].data);
		Helpers::panic("SendSyncRequest targetting port %s\n", portData->name);
	}

	regs[0] = SVCResult::Success;
}