/*
MIT License

Copyright (c) 2016 Benjamin "Nefarius" H�glinger

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/


#include "driver.h"
#include "device.tmh"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, HidGuardianCreateDevice)
#pragma alloc_text (PAGE, EvtDeviceFileCreate)
#pragma alloc_text (PAGE, AmIAffected)
#pragma alloc_text (PAGE, AmIWhitelisted)
#endif


NTSTATUS
HidGuardianCreateDevice(
    _Inout_ PWDFDEVICE_INIT DeviceInit
)
{
    WDF_OBJECT_ATTRIBUTES   deviceAttributes;
    PDEVICE_CONTEXT         deviceContext;
    WDFDEVICE               device;
    NTSTATUS                status;
    WDF_FILEOBJECT_CONFIG   deviceConfig;
    WDFMEMORY               memory;

    PAGED_CODE();

    WdfFdoInitSetFilter(DeviceInit);

    WDF_OBJECT_ATTRIBUTES_INIT(&deviceAttributes);
    deviceAttributes.SynchronizationScope = WdfSynchronizationScopeNone;
    WDF_FILEOBJECT_CONFIG_INIT(&deviceConfig, EvtDeviceFileCreate, NULL, NULL);

    WdfDeviceInitSetFileObjectConfig(
        DeviceInit,
        &deviceConfig,
        &deviceAttributes
    );

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&deviceAttributes, DEVICE_CONTEXT);

    //
    // We will just register for cleanup notification because we have to
    // delete the control-device when the last instance of the device goes
    // away. If we don't delete, the driver wouldn't get unloaded automatically
    // by the PNP subsystem.
    //
    deviceAttributes.EvtCleanupCallback = HidGuardianEvtDeviceContextCleanup;

    status = WdfDeviceCreate(&DeviceInit, &deviceAttributes, &device);

    if (NT_SUCCESS(status)) {
        //
        // Get a pointer to the device context structure that we just associated
        // with the device object. We define this structure in the device.h
        // header file. DeviceGetContext is an inline function generated by
        // using the WDF_DECLARE_CONTEXT_TYPE_WITH_NAME macro in device.h.
        // This function will do the type checking and return the device context.
        // If you pass a wrong object handle it will return NULL and assert if
        // run under framework verifier mode.
        //
        deviceContext = DeviceGetContext(device);

        if (!NT_SUCCESS(status)) {
            KdPrint((DRIVERNAME "WdfDeviceCreateDeviceInterface failed with status 0x%X", status));
            return status;
        }

        WDF_OBJECT_ATTRIBUTES_INIT(&deviceAttributes);
        deviceAttributes.ParentObject = device;

        //
        // Query for current device's Hardware ID
        // 
        status = WdfDeviceAllocAndQueryProperty(device,
            DevicePropertyHardwareID,
            NonPagedPool,
            &deviceAttributes,
            &memory
        );

        if (!NT_SUCCESS(status)) {
            KdPrint((DRIVERNAME "WdfDeviceAllocAndQueryProperty failed with status 0x%X", status));
            return status;
        }

        //
        // Get Hardware ID string
        // 
        deviceContext->HardwareIDsMemory = memory;
        deviceContext->HardwareIDs = WdfMemoryGetBuffer(memory, NULL);

        //
        // Initialize the I/O Package and any Queues
        //
        status = HidGuardianQueueInitialize(device);

        if (!NT_SUCCESS(status)) {
            KdPrint((DRIVERNAME "HidGuardianQueueInitialize failed with status 0x%X", status));
            return status;
        }

        //
        // Add this device to the FilterDevice collection.
        //
        WdfWaitLockAcquire(FilterDeviceCollectionLock, NULL);
        //
        // WdfCollectionAdd takes a reference on the item object and removes
        // it when you call WdfCollectionRemove.
        //
        status = WdfCollectionAdd(FilterDeviceCollection, device);
        if (!NT_SUCCESS(status)) {
            KdPrint(("WdfCollectionAdd failed with status code 0x%x\n", status));
        }
        WdfWaitLockRelease(FilterDeviceCollectionLock);

        //
        // Create a control device
        //
        status = HidGuardianCreateControlDevice(device);
        if (!NT_SUCCESS(status)) {
            KdPrint(("HidGuardianCreateControlDevice failed with status 0x%x\n",
                status));
            //
            // Let us not fail AddDevice just because we weren't able to create the
            // control device.
            //
            status = STATUS_SUCCESS;
        }

        //
        // Check if this device should get intercepted
        // 
        status = AmIAffected(deviceContext);

        KdPrint((DRIVERNAME "AmIAffected status 0x%X\n", status));
    }

    return status;
}

#pragma warning(push)
#pragma warning(disable:28118) // this callback will run at IRQL=PASSIVE_LEVEL
_Use_decl_annotations_
VOID
HidGuardianEvtDeviceContextCleanup(
    WDFOBJECT Device
)
/*++

Routine Description:

EvtDeviceRemove event callback must perform any operations that are
necessary before the specified device is removed. The framework calls
the driver's EvtDeviceRemove callback when the PnP manager sends
an IRP_MN_REMOVE_DEVICE request to the driver stack.

Arguments:

Device - Handle to a framework device object.

Return Value:

WDF status code

--*/
{
    ULONG   count;

    PAGED_CODE();

    KdPrint(("Entered HidGuardianEvtDeviceContextCleanup\n"));

    WdfWaitLockAcquire(FilterDeviceCollectionLock, NULL);

    count = WdfCollectionGetCount(FilterDeviceCollection);

    if (count == 1)
    {
        //
        // We are the last instance. So let us delete the control-device
        // so that driver can unload when the FilterDevice is deleted.
        // We absolutely have to do the deletion of control device with
        // the collection lock acquired because we implicitly use this
        // lock to protect ControlDevice global variable. We need to make
        // sure another thread doesn't attempt to create while we are
        // deleting the device.
        //
        HidGuardianDeleteControlDevice((WDFDEVICE)Device);
    }

    WdfCollectionRemove(FilterDeviceCollection, Device);

    WdfWaitLockRelease(FilterDeviceCollectionLock);
}
#pragma warning(pop) // enable 28118 again

//
// Catches CreateFile(...) calls.
// 
VOID EvtDeviceFileCreate(
    _In_ WDFDEVICE     Device,
    _In_ WDFREQUEST    Request,
    _In_ WDFFILEOBJECT FileObject
)
{
    DWORD                           pid;
    WDF_REQUEST_SEND_OPTIONS        options;
    NTSTATUS                        status;
    BOOLEAN                         ret;

    UNREFERENCED_PARAMETER(FileObject);

    PAGED_CODE();

    pid = CURRENT_PROCESS_ID();

    if (AmIWhitelisted(pid))
    {
        WdfRequestFormatRequestUsingCurrentType(Request);

        //
        // PID is white-listed, pass request down the stack
        // 
        WDF_REQUEST_SEND_OPTIONS_INIT(&options,
            WDF_REQUEST_SEND_OPTION_SEND_AND_FORGET);

        ret = WdfRequestSend(Request, WdfDeviceGetIoTarget(Device), &options);

        if (ret == FALSE) {
            status = WdfRequestGetStatus(Request);
            KdPrint((DRIVERNAME "WdfRequestSend failed: 0x%x\n", status));
            WdfRequestComplete(Request, status);
        }
    }
    else
    {
        //
        // PID is not white-listed, fail the open request
        // 
        KdPrint((DRIVERNAME "CreateFile(...) blocked for PID: %d\n", pid));
        WdfRequestComplete(Request, STATUS_ACCESS_DENIED);
    }
}

