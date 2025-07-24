using Microsoft.Win32.SafeHandles;
using System;
using System.ComponentModel;
using System.Runtime.InteropServices;

namespace PloutonLogViewer
{
    /// <summary>
    /// Reads physical memory using the RwDrv.sys driver from RWEverything.
    /// This implementation is based on the IOCTL interface using a pointer structure.
    /// </summary>
    public class PhysicalMemoryReader : IDisposable
    {
        private const uint GenericRead = 0x80000000;
        private const uint GenericWrite = 0x40000000;
        private const uint FileShareRead = 1;
        private const uint FileShareWrite = 2;
        private const uint OpenExisting = 3;

        /// <summary>
        /// IOCTL for RwDrv.sys to read a block from physical memory.
        /// </summary>
        private const uint IoctlReadPhysicalMemory = 0x222808;

        private SafeFileHandle _handle;

        [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
        private static extern SafeFileHandle CreateFile(
            string lpFileName,
            uint dwDesiredAccess,
            uint dwShareMode,
            IntPtr lpSecurityAttributes,
            uint dwCreationDisposition,
            uint dwFlagsAndAttributes,
            IntPtr hTemplateFile);

        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern bool DeviceIoControl(
            SafeFileHandle hDevice,
            uint dwIoControlCode,
            IntPtr lpInBuffer,
            uint nInBufferSize,
            IntPtr lpOutBuffer,
            uint nOutBufferSize,
            out uint lpBytesReturned,
            IntPtr lpOverlapped);
        
        /// <summary>
        /// The structure passed to DeviceIoControl to request a physical memory operation.
        /// This mirrors the PhysRw_t structure from the C++ example.
        /// </summary>
        [StructLayout(LayoutKind.Sequential)]
        private struct PhysicalReadWriteRequest
        {
            public ulong PhysicalAddress;
            public uint Size;   // In units of Access. For byte read, this is the count of bytes.
            public uint Access; // 0 for byte, 1 for word, 2 for dword
            public ulong Buffer; // Pointer to the user-space buffer
        }

        public void Initialize()
        {
            // The device name for the RwDrv.sys driver.
            _handle = CreateFile(
                @"\\.\RwDrv",
                GenericRead | GenericWrite,
                FileShareRead | FileShareWrite,
                IntPtr.Zero,
                OpenExisting,
                0,
                IntPtr.Zero);

            if (_handle.IsInvalid)
            {
                var errorCode = Marshal.GetLastWin32Error();
                throw new Win32Exception(errorCode, $"The handle of the RwDrv.sys driver cannot be opened. Please ensure that RWEverything is installed or that the RwDrv.sys driver has been successfully loaded. Error code: {errorCode}");
            }
        }

        /// <summary>
        /// Reads a block of data from physical memory into the provided buffer.
        /// </summary>
        /// <param name="address">The starting physical address.</param>
        /// <param name="buffer">The buffer to fill with data.</param>
        public void ReadPhysicalMemory(ulong address, byte[] buffer)
        {
            if (_handle == null || _handle.IsInvalid)
            {
                throw new InvalidOperationException("The driver is not initialized or the handle is invalid。");
            }
            if (buffer == null || buffer.Length == 0)
            {
                throw new ArgumentException("Buffer cannot be null or empty.", nameof(buffer));
            }

            // Pin the managed buffer in memory so the GC doesn't move it, and get its address.
            GCHandle pinnedBuffer = GCHandle.Alloc(buffer, GCHandleType.Pinned);
            IntPtr pBuffer = pinnedBuffer.AddrOfPinnedObject();

            // This is the structure we send to the driver.
            var request = new PhysicalReadWriteRequest
            {
                PhysicalAddress = address,
                Access = 0, // 0 = byte access
                Size = (uint)buffer.Length,
                Buffer = (ulong)pBuffer.ToInt64()
            };
            
            // Allocate unmanaged memory for the request structure itself.
            var pRequest = Marshal.AllocHGlobal(Marshal.SizeOf<PhysicalReadWriteRequest>());

            try
            {
                Marshal.StructureToPtr(request, pRequest, false);

                if (!DeviceIoControl(
                        _handle,
                        IoctlReadPhysicalMemory,
                        pRequest,
                        (uint)Marshal.SizeOf<PhysicalReadWriteRequest>(),
                        pRequest,
                        (uint)Marshal.SizeOf<PhysicalReadWriteRequest>(),
                        out _,
                        IntPtr.Zero))
                {
                    var errorCode = Marshal.GetLastWin32Error();
                    throw new Win32Exception(errorCode, $"Read the physical address 0x{address:X} Failure。Error code: {errorCode}");
                }

                // Data is read directly into our pinned `buffer`. No further action is needed.
            }
            finally
            {
                // ALWAYS free the allocated memory and unpin the handle.
                Marshal.FreeHGlobal(pRequest);
                pinnedBuffer.Free();
            }
        }

        public void Deinitialize()
        {
            _handle?.Dispose();
        }

        public void Dispose()
        {
            Deinitialize();
            GC.SuppressFinalize(this);
        }
    }
} 