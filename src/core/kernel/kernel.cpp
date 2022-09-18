#include <cassert>
#include "kernel.hpp"
#include "kernel_types.hpp"

void Kernel::serviceSVC(u32 svc) {
	switch (svc) {
		case 0x01: controlMemory(); break;
		case 0x21: createAddressArbiter(); break;
		case 0x23: svcCloseHandle(); break;
		case 0x2D: connectToPort(); break;
		case 0x32: sendSyncRequest(); break;
		case 0x38: getResourceLimit(); break;
		case 0x39: getResourceLimitLimitValues(); break;
		case 0x3A: getResourceLimitCurrentValues(); break;
		case 0x3D: outputDebugString(); break;
		default: Helpers::panic("Unimplemented svc: %X @ %08X", svc, regs[15]); break;
	}
}

Handle Kernel::makeProcess() {
	const Handle processHandle = makeObject(KernelObjectType::Process);
	const Handle resourceLimitHandle = makeObject(KernelObjectType::ResourceLimit);

	// Allocate data
	objects[processHandle].data = new ProcessData();
	const auto processData = static_cast<ProcessData*>(objects[processHandle].data);

	// Link resource limit object with its parent process
	objects[resourceLimitHandle].data = &processData->limits;
	processData->limits.handle = resourceLimitHandle;
	return processHandle;
}

// Get a pointer to the process indicated by handle, taking into account that 0xFFFF8001 always refers to the current process
// Returns nullptr if the handle does not correspond to a process
KernelObject* Kernel::getProcessFromPID(Handle handle) {
	if (handle == KernelHandles::CurrentProcess) [[likely]] {
		return getObject(currentProcess, KernelObjectType::Process);
	} else {
		return getObject(handle, KernelObjectType::Process);
	}
}

void Kernel::deleteObjectData(KernelObject& object) {
	using enum KernelObjectType;

	// Resource limit, service and dummy objects do not allocate heap data, so we don't delete anything
	if (object.data == nullptr || object.type == ResourceLimit || object.type == Dummy) {
		return;
	}

	delete object.data;
}

void Kernel::reset() {
	handleCounter = 0;

	for (auto& object : objects) {
		deleteObjectData(object);
	}
	objects.clear();
	portHandles.clear();
	serviceManager.reset();

	// Allocate handle #0 to a dummy object and make a main process object
	makeObject(KernelObjectType::Dummy);
	currentProcess = makeProcess();

	// Create global service manager port
	makePort("srv:");
}

// Get pointer to thread-local storage
// TODO: Every thread should have its own TLS. We need to adjust for this when we add threads
u32 Kernel::getTLSPointer() {
	return VirtualAddrs::TLSBase;
}

// Result CreateAddressArbiter(Handle* arbiter)
void Kernel::createAddressArbiter() {
	printf("Stubbed call to CreateAddressArbiter. Handle address: %08X\n", regs[0]);
	regs[0] = SVCResult::Success;
}

// Result CloseHandle(Handle handle)
void Kernel::svcCloseHandle() {
	printf("CloseHandle(handle = %d) (Unimplemented)\n", regs[0]);
	regs[0] = SVCResult::Success;
}

// OutputDebugString(const char* str, int size)
// TODO: Does this actually write an error code in r0?
void Kernel::outputDebugString() {
	const u32 pointer = regs[0];
	const u32 size = regs[1];

	std::string message = mem.readString(pointer, size);
	printf("OutputDebugString(message = \"%s\")\n", message.c_str());
	regs[0] = SVCResult::Success;
}

std::string Kernel::getProcessName(u32 pid) {
	if (pid == KernelHandles::CurrentProcess) {
		return "current";
	} else {
		Helpers::panic("Attempted to name non-current process");
	}
}