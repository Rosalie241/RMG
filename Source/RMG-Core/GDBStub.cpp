// Taken from https://github.com/Gillou68310/mupen64plus-ui-console/blob/gdbstub/src/gdbstub.cpp
// And modified for RMG-Core

// Copyright 2013 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

// Originally written by Sven Peter <sven@fail0verflow.com> for anergistic.

#include <algorithm>
#include <atomic>
#include <climits>
#include <csignal>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <array>
#include <vector>
#include <cstdint>
#include <map>
#include <numeric>
#include <string>
#include <fcntl.h>

#ifdef USE_LIBFMT
#include "../3rdParty/fmt/include/fmt/core.h"
#include "../3rdParty/fmt/include/fmt/format.h"
#include "../3rdParty/fmt/include/fmt/format-inl.h"
#include "../3rdParty/fmt/src/format.cc"

#define fmt_string(...) fmt_string(__VA_ARGS__)
#else // USE_LIBFMT
#include <format>

#define fmt_string(...) std::format(__VA_ARGS__)
#endif // USE_LIBFMT

#ifdef _WIN32
#include <winsock2.h>
// winsock2.h needs to be included first to prevent winsock.h being included by other includes
#include <io.h>
#include <iphlpapi.h>
#include <ws2tcpip.h>
#define SHUT_RDWR 2
#else
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#endif

#include "GDBStub.hpp"
#include "m64p/Api.hpp"

//
// Local Defines
//

#define LOG_ERROR(...) print_message(M64MSG_ERROR, fmt_string( __VA_ARGS__ ).c_str())
#define LOG_DEBUG(...) print_message(M64MSG_VERBOSE, fmt_string( __VA_ARGS__ ).c_str())
#define LOG_INFO(...) print_message(M64MSG_INFO, fmt_string( __VA_ARGS__ ).c_str())

#define GDB_BUFFER_SIZE 10000
#define GDB_STUB_START '$'
#define GDB_STUB_END '#'
#define GDB_STUB_ACK '+'
#define GDB_STUB_NACK '-'

#ifndef SIGTRAP
#define SIGTRAP 5
#endif

#ifndef SIGTERM
#define SIGTERM 15
#endif

#ifndef MSG_WAITALL
#define MSG_WAITALL 8
#endif

#define SP_REGISTER 29
#define STATUS_REGISTER 32
#define LO_REGISTER 33
#define HI_REGISTER 34
#define BADVADDR_REGISTER 35
#define CAUSE_REGISTER 36
#define PC_REGISTER 37
#define FCSR_REGISTER 70
#define FIR_REGISTER 71

//
// Local Variables
//

// For sample XML files see the GDB source /gdb/features
// GDB also wants the l character at the start
// This XML defines what the registers are for this specific ARM device
static char target_xml[] =
    R"(l<?xml version="1.0"?>
<!DOCTYPE target SYSTEM "gdb-target.dtd">
<target version="1.0">
	<architecture>mips:4300</architecture>
	<feature name="org.gnu.gdb.mips.cpu">
		<reg name="r0" bitsize="64" regnum="0"/>
		<reg name="r1" bitsize="64"/>
		<reg name="r2" bitsize="64"/>
		<reg name="r3" bitsize="64"/>
		<reg name="r4" bitsize="64"/>
		<reg name="r5" bitsize="64"/>
		<reg name="r6" bitsize="64"/>
		<reg name="r7" bitsize="64"/>
		<reg name="r8" bitsize="64"/>
		<reg name="r9" bitsize="64"/>
		<reg name="r10" bitsize="64"/>
		<reg name="r11" bitsize="64"/>
		<reg name="r12" bitsize="64"/>
		<reg name="r13" bitsize="64"/>
		<reg name="r14" bitsize="64"/>
		<reg name="r15" bitsize="64"/>
		<reg name="r16" bitsize="64"/>
		<reg name="r17" bitsize="64"/>
		<reg name="r18" bitsize="64"/>
		<reg name="r19" bitsize="64"/>
		<reg name="r20" bitsize="64"/>
		<reg name="r21" bitsize="64"/>
		<reg name="r22" bitsize="64"/>
		<reg name="r23" bitsize="64"/>
		<reg name="r24" bitsize="64"/>
		<reg name="r25" bitsize="64"/>
		<reg name="r26" bitsize="64"/>
		<reg name="r27" bitsize="64"/>
		<reg name="r28" bitsize="64"/>
		<reg name="r29" bitsize="64"/>
		<reg name="r30" bitsize="64"/>
		<reg name="r31" bitsize="64"/>
		<reg name="lo" bitsize="64" regnum="33"/>
		<reg name="hi" bitsize="64" regnum="34"/>
		<reg name="pc" bitsize="64" regnum="37"/>
	</feature>
	<feature name="org.gnu.gdb.mips.fpu">
		<reg name="f0" bitsize="64" type="ieee_double" regnum="38"/>
		<reg name="f1" bitsize="64" type="ieee_double"/>
		<reg name="f2" bitsize="64" type="ieee_double"/>
		<reg name="f3" bitsize="64" type="ieee_double"/>
		<reg name="f4" bitsize="64" type="ieee_double"/>
		<reg name="f5" bitsize="64" type="ieee_double"/>
		<reg name="f6" bitsize="64" type="ieee_double"/>
		<reg name="f7" bitsize="64" type="ieee_double"/>
		<reg name="f8" bitsize="64" type="ieee_double"/>
		<reg name="f9" bitsize="64" type="ieee_double"/>
		<reg name="f10" bitsize="64" type="ieee_double"/>
		<reg name="f11" bitsize="64" type="ieee_double"/>
		<reg name="f12" bitsize="64" type="ieee_double"/>
		<reg name="f13" bitsize="64" type="ieee_double"/>
		<reg name="f14" bitsize="64" type="ieee_double"/>
		<reg name="f15" bitsize="64" type="ieee_double"/>
		<reg name="f16" bitsize="64" type="ieee_double"/>
		<reg name="f17" bitsize="64" type="ieee_double"/>
		<reg name="f18" bitsize="64" type="ieee_double"/>
		<reg name="f19" bitsize="64" type="ieee_double"/>
		<reg name="f20" bitsize="64" type="ieee_double"/>
		<reg name="f21" bitsize="64" type="ieee_double"/>
		<reg name="f22" bitsize="64" type="ieee_double"/>
		<reg name="f23" bitsize="64" type="ieee_double"/>
		<reg name="f24" bitsize="64" type="ieee_double"/>
		<reg name="f25" bitsize="64" type="ieee_double"/>
		<reg name="f26" bitsize="64" type="ieee_double"/>
		<reg name="f27" bitsize="64" type="ieee_double"/>
		<reg name="f28" bitsize="64" type="ieee_double"/>
		<reg name="f29" bitsize="64" type="ieee_double"/>
		<reg name="f30" bitsize="64" type="ieee_double"/>
		<reg name="f31" bitsize="64" type="ieee_double"/>
		<reg name="fcsr" bitsize="64" group="float"/>
		<reg name="fir" bitsize="64" group="float"/>
	</feature>
	<feature name="org.gnu.gdb.mips.cp0">
		<reg name="status" bitsize="64" regnum="32"/>
		<reg name="badvaddr" bitsize="64" regnum="35"/>
		<reg name="cause" bitsize="64" regnum="36"/>
	</feature>
</target>
)";

static int gdbserver_socket = -1;

static uint8_t command_buffer[GDB_BUFFER_SIZE];
static uint32_t command_length;

static uint32_t latest_signal = 0;
static bool first_exception = true;

#ifdef _WIN32
static WSADATA InitData;
#endif

//
// Local Functions
//

#include <iostream>
static void print_message(int level, const char* msg)
{
    // TODO: use proper callback
    //DebugCallback((void*)"GDBStub", level, msg);
    std::cout << "[GDBStub] " << msg << std::endl;
}

static uint64_t RegRead(std::size_t id)
{
    if (id < 32)
    {
        uint64_t* regs = (uint64_t*)m64p::Debug.GetCPUDataPtr(M64P_CPU_REG_REG);
        return regs[id];
    }
    else if (id == STATUS_REGISTER)
    {
        uint32_t* cp0 = (uint32_t*)m64p::Debug.GetCPUDataPtr(M64P_CPU_REG_COP0);
        return (uint64_t)cp0[12];
    }
    else if (id == LO_REGISTER)
    {
        return *(uint64_t*)m64p::Debug.GetCPUDataPtr(M64P_CPU_REG_LO);
    }
    else if (id == HI_REGISTER)
    {
        return *(uint64_t*)m64p::Debug.GetCPUDataPtr(M64P_CPU_REG_HI);
    }
    else if (id == BADVADDR_REGISTER)
    {
        uint32_t* cp0 = (uint32_t*)m64p::Debug.GetCPUDataPtr(M64P_CPU_REG_COP0);
        return (uint64_t)cp0[8];
    }
    else if (id == CAUSE_REGISTER)
    {
        uint32_t* cp0 = (uint32_t*)m64p::Debug.GetCPUDataPtr(M64P_CPU_REG_COP0);
        return (uint64_t)cp0[13];
    }
    else if (id == PC_REGISTER)
    {
        return (uint64_t)*(int32_t*)m64p::Debug.GetCPUDataPtr(M64P_CPU_PC);
    }
    else if (id >= 38 && id < FCSR_REGISTER)
    {
        uint64_t* cp1 = (uint64_t*)m64p::Debug.GetCPUDataPtr(M64P_CPU_REG_COP1_FGR_64);
        return cp1[id - 38];
    }
    else if (id == FCSR_REGISTER)
    {
        return 0; //TODO
    }
    else if (id == FIR_REGISTER)
    {
        return 0; //TODO
    }
    else
    {
        return 0;
    }
}

static void RegWrite(std::size_t id, int64_t val) {
    if (id < 32)
    {
        int64_t* regs = (int64_t*)m64p::Debug.GetCPUDataPtr(M64P_CPU_REG_REG);
        regs[id] = val;
    }
    else if (id == STATUS_REGISTER)
    {
        int64_t* cp0 = (int64_t*)m64p::Debug.GetCPUDataPtr(M64P_CPU_REG_COP0);
        cp0[12] = static_cast<int64_t>(val);
    }
    else if (id == LO_REGISTER)
    {
        *(int64_t*)m64p::Debug.GetCPUDataPtr(M64P_CPU_REG_LO) = val;
    }
    else if (id == HI_REGISTER)
    {
        *(int64_t*)m64p::Debug.GetCPUDataPtr(M64P_CPU_REG_HI) = val;
    }
    else if (id == BADVADDR_REGISTER)
    {
        int64_t* cp0 = (int64_t*)m64p::Debug.GetCPUDataPtr(M64P_CPU_REG_COP0);
        cp0[8] = static_cast<int64_t>(val);
    }
    else if (id == CAUSE_REGISTER)
    {
        int64_t* cp0 = (int64_t*)m64p::Debug.GetCPUDataPtr(M64P_CPU_REG_COP0);
        cp0[13] = static_cast<int64_t>(val);
    }
    else if (id == PC_REGISTER)
    {
        *(uint32_t*)m64p::Debug.GetCPUDataPtr(M64P_CPU_PC) = static_cast<uint32_t>(val);
    }
    else if (id >= 38 && id < FCSR_REGISTER)
    {
        int64_t* cp1 = (int64_t*)m64p::Debug.GetCPUDataPtr(M64P_CPU_REG_COP1_FGR_64);
        cp1[id - 38] = val;
    }
    else if (id == FCSR_REGISTER)
    {
        //TODO
    }
    else if (id == FIR_REGISTER)
    {
        //TODO
    }
}

/**
 * Turns hex string character into the equivalent byte.
 *
 * @param hex Input hex character to be turned into byte.
 */
static uint8_t HexCharToValue(uint8_t hex)
{
    if (hex >= '0' && hex <= '9') {
        return hex - '0';
    } else if (hex >= 'a' && hex <= 'f') {
        return hex - 'a' + 0xA;
    } else if (hex >= 'A' && hex <= 'F') {
        return hex - 'A' + 0xA;
    }

    LOG_ERROR("Invalid nibble: {} ({:02X})", hex, hex);
    return 0;
}

/**
 * Turn nibble of byte into hex string character.
 *
 * @param n Nibble to be turned into hex character.
 */
static uint8_t NibbleToHex(uint8_t n)
{
    n &= 0xF;
    if (n < 0xA) {
        return '0' + n;
    } else {
        return 'a' + n - 0xA;
    }
}

/**
 * Converts input hex string characters into an array of equivalent of uint8_t bytes.
 *
 * @param src Pointer to array of output hex string characters.
 * @param len Length of src array.
 */
static uint32_t HexToInt(const uint8_t* src, std::size_t len)
{
    uint32_t output = 0;
    while (len-- > 0) {
        output = (output << 4) | HexCharToValue(src[0]);
        src++;
    }
    return output;
}

/**
 * Converts input hex string characters into an array of equivalent of uint8_t bytes.
 *
 * @param src Pointer to array of output hex string characters.
 * @param len Length of src array.
 */
static uint64_t HexToLong(const uint8_t* src, std::size_t len) {
    uint64_t output = 0;
    while (len-- > 0) {
        output = (output << 4) | HexCharToValue(src[0]);
        src++;
    }
    return output;
}

/**
 * Converts input array of uint8_t bytes into their equivalent hex string characters.
 *
 * @param dest Pointer to buffer to store output hex string characters.
 * @param src Pointer to array of uint8_t bytes.
 * @param len Length of src array.
 */
static void MemToGdbHex(uint8_t* dest, const uint8_t* src, std::size_t len) {
    while (len-- > 0)
    {
        uint8_t tmp = *src++;
        *dest++ = NibbleToHex(tmp >> 4);
        *dest++ = NibbleToHex(tmp);
    }
}

/**
 * Converts input gdb-formatted hex string characters into an array of equivalent of uint8_t bytes.
 *
 * @param dest Pointer to buffer to store uint8_t bytes.
 * @param src Pointer to array of output hex string characters.
 * @param len Length of src array.
 */
static void GdbHexToMem(uint8_t* dest, const uint8_t* src, std::size_t len) {
    while (len-- > 0)
    {
        *dest++ = (HexCharToValue(src[0]) << 4) | HexCharToValue(src[1]);
        src += 2;
    }
}

/**
 * Convert a uint32_t into a gdb-formatted hex string.
 *
 * @param dest Pointer to buffer to store output hex string characters.
 * @param v    Value to convert.
 */
static void IntToGdbHex(uint8_t* dest, uint32_t v) {
    for (int i = 0; i < 8; i += 2)
    {
        dest[i]   = NibbleToHex(static_cast<uint8_t>(v >> (28-i*4)));
        dest[i+1] = NibbleToHex(static_cast<uint8_t>(v >> (28-(i+1)*4)));
    }
}

/**
 * Convert a uint64_t into a gdb-formatted hex string.
 *
 * @param dest Pointer to buffer to store output hex string characters.
 * @param v    Value to convert.
 */
static void LongToGdbHex(uint8_t* dest, uint64_t v)
{
    for (int i = 0; i < 16; i += 2)
    {
        dest[i] = NibbleToHex(static_cast<uint8_t>(v >> (60-i*4)));
        dest[i+1] = NibbleToHex(static_cast<uint8_t>(v >> (60-(i+1)*4)));
    }
}

/**
 * Convert a gdb-formatted hex string into a uint32_t.
 *
 * @param src Pointer to hex string.
 */
static uint32_t GdbHexToInt(const uint8_t* src)
{
    uint32_t output = 0;

    for (int i = 0; i < 8; i += 2) {
        output = (output << 4) | HexCharToValue(src[i]);
        output = (output << 4) | HexCharToValue(src[i+1]);
    }

    return output;
}

/**
 * Convert a gdb-formatted hex string into a uint64_t.
 *
 * @param src Pointer to hex string.
 */
static uint64_t GdbHexToLong(const uint8_t* src)
{
    uint64_t output = 0;

    for (int i = 0; i < 16; i += 2) {
        output = (output << 4) | HexCharToValue(src[i]);
        output = (output << 4) | HexCharToValue(src[i+1]);
    }

    return output;
}

/// Read a byte from the gdb client.
static uint8_t ReadByte()
{
    uint8_t c;
    std::size_t received_size = recv(gdbserver_socket, reinterpret_cast<char*>(&c), 1, MSG_WAITALL);
    if (received_size != 1) {
        LOG_ERROR("recv failed: {}", received_size);
        CoreGDBStubShutdown();
    }

    return c;
}

/// Calculate the checksum of the current command buffer.
static uint8_t CalculateChecksum(const uint8_t* buffer, std::size_t length)
{
    return static_cast<uint8_t>(std::accumulate(buffer, buffer + length, 0, std::plus<uint8_t>()));
}

/**
 * Remove the breakpoint from the given address of the specified type.
 *
 * @param type Type of breakpoint.
 * @param addr Address of breakpoint.
 */
static bool RemoveBreakpoint(uint8_t type, uint32_t addr)
{
    uint32_t flags = M64P_BKP_FLAG_ENABLED | M64P_BKP_FLAG_LOG;

    switch (type) {
    case 0:
    case 1:
        flags |= M64P_BKP_FLAG_EXEC;
        break;
    case 2:
        flags |= M64P_BKP_FLAG_WRITE;
        break;
    case 3:
        flags |= M64P_BKP_FLAG_READ;
        break;
    case 4:
        flags |= M64P_BKP_FLAG_WRITE | M64P_BKP_FLAG_READ;
        break;
    default:
        return false;
    }

    int numBkps = m64p::Debug.BreakpointLookup(addr, 1, flags);
    if (numBkps < 0)
    {
        LOG_ERROR("Trying to remove non-existing breakpoint");
        return false;
    }

    LOG_DEBUG("gdb: removed a breakpoint: {:016X} bytes at {:016X} of type {}",
              4, addr, static_cast<int>(type));

    m64p::Debug.BreakpointCommand(M64P_BKP_CMD_REMOVE_IDX, numBkps, NULL);
    return true;
}

static void gdstub_sendpacket(const char packet) {
    std::size_t sent_size = send(gdbserver_socket, &packet, 1, 0);
    if (sent_size != 1)
    {
        LOG_ERROR("send failed");
    }
}

static void gdbstub_sendreply(const char* reply) {
    LOG_DEBUG("Reply: {}", reply);

    memset(command_buffer, 0, sizeof(command_buffer));

    command_length = static_cast<uint32_t>(strlen(reply));
    if (command_length + 4 > sizeof(command_buffer))
    {
        LOG_ERROR("command_buffer overflow in gdbstub_sendreply");
        return;
    }

    memcpy(command_buffer + 1, reply, command_length);

    uint8_t checksum = CalculateChecksum(command_buffer, command_length + 1);
    command_buffer[0] = GDB_STUB_START;
    command_buffer[command_length + 1] = GDB_STUB_END;
    command_buffer[command_length + 2] = NibbleToHex(checksum >> 4);
    command_buffer[command_length + 3] = NibbleToHex(checksum);

    uint8_t* ptr = command_buffer;
    uint32_t left = command_length + 4;
    while (left > 0)
    {
        int sent_size = send(gdbserver_socket, reinterpret_cast<char*>(ptr), left, 0);
        if (sent_size < 0)
        {
            LOG_ERROR("gdb: send failed");
            CoreGDBStubShutdown();
            return;
        }

        left -= sent_size;
        ptr  += sent_size;
    }
}

/// Handle query command from gdb client.
static void HandleQuery()
{
    LOG_DEBUG("gdb: query '{}'", (char*)(command_buffer + 1));

    const char* query = reinterpret_cast<const char*>(command_buffer + 1);

    if (strcmp(query, "TStatus") == 0)
    {
        gdbstub_sendreply("");
    }
    else if (strcmp(query, "Attached") == 0)
    {
        gdbstub_sendreply("1");
    }
    else if (strncmp(query, "Supported", strlen("Supported")) == 0)
    {
        // PacketSize needs to be large enough for target xml
        gdbstub_sendreply("PacketSize=2000;qXfer:features:read+");
    }
    else if (strncmp(query, "Xfer:features:read:target.xml:",
                       strlen("Xfer:features:read:target.xml:")) == 0)
    {
        gdbstub_sendreply(target_xml);
    }
    else if (strncmp(query, "Offsets", strlen("Offsets")) == 0)
    {
        const uint32_t base_address = 0;
        std::string buffer = fmt_string("TextSeg={:0x}", base_address);
        gdbstub_sendreply(buffer.c_str());
    }
    else if (strncmp(query, "fThreadInfo", strlen("fThreadInfo")) == 0)
    {
        gdbstub_sendreply("m0");
    }
    else if (strncmp(query, "sThreadInfo", strlen("sThreadInfo")) == 0)
    {
        gdbstub_sendreply("l");
    }
    else 
    {
        gdbstub_sendreply("");
    }
}

/// Handle set thread command from gdb client.
static void HandleSetThread()
{
    gdbstub_sendreply("OK");
}

/// Handle thread alive command from gdb client.
static void HandleThreadAlive()
{
    gdbstub_sendreply("OK");
}

/**
 * Send signal packet to client.
 *
 * @param signal Signal to be sent to client.
 */
static void SendSignal(uint32_t signal, bool full = true)
{
    latest_signal = signal;

    std::string buffer;
    if (full) {
        buffer = fmt_string("T{:02x}{:02x}:{:016x};{:02x}:{:016x};", latest_signal,
                             PC_REGISTER, RegRead(PC_REGISTER),
                             SP_REGISTER, RegRead(SP_REGISTER));
    } else {
        buffer = fmt_string("T{:02x}", latest_signal);
    }

    gdbstub_sendreply(buffer.c_str());
}

/// Read command from gdb client.
static void ReadCommand()
{
    command_length = 0;
    memset(command_buffer, 0, sizeof(command_buffer));

    uint8_t c = ReadByte();
    if (c == '+')
    {
        // ignore ack
        return;
    }
    else if (c == 0x03)
    {
        LOG_INFO("gdb: found break command");
        m64p::Debug.SetRunState(M64P_DBG_RUNSTATE_PAUSED);
        SendSignal(SIGTRAP);
        return;
    }
    else if (c != GDB_STUB_START)
    {
        LOG_DEBUG("gdb: read invalid byte {:02X}", c);
        return;
    }

    while ((c = ReadByte()) != GDB_STUB_END)
    {
        if (command_length >= sizeof(command_buffer))
        {
            LOG_ERROR("gdb: command_buffer overflow");
            gdstub_sendpacket(GDB_STUB_NACK);
            return;
        }
        command_buffer[command_length++] = c;
    }

    uint8_t checksum_received = HexCharToValue(ReadByte()) << 4;
    checksum_received |= HexCharToValue(ReadByte());

    uint8_t checksum_calculated = CalculateChecksum(command_buffer, command_length);
    if (checksum_received != checksum_calculated)
    {
        LOG_ERROR("gdb: invalid checksum: calculated {:02X} and read {:02X} for ${}# (length: {})",
                  checksum_calculated, checksum_received, (char*)command_buffer, command_length);

        command_length = 0;

        gdstub_sendpacket(GDB_STUB_NACK);
        return;
    }

    gdstub_sendpacket(GDB_STUB_ACK);
}

/// Check if there is data to be read from the gdb client.
static bool IsDataAvailable()
{
    fd_set fd_socket;

    FD_ZERO(&fd_socket);
    FD_SET(static_cast<uint32_t>(gdbserver_socket), &fd_socket);

    struct timeval t;
    t.tv_sec = 0;
    t.tv_usec = 0;

    if (select(gdbserver_socket + 1, &fd_socket, nullptr, nullptr, &t) < 0)
    {
        LOG_ERROR("select failed");
        return false;
    }

    return FD_ISSET(gdbserver_socket, &fd_socket) != 0;
}

/// Send requested register to gdb client.
static void ReadRegister()
{
    static uint8_t reply[64];
    memset(reply, 0, sizeof(reply));

    uint32_t id = HexCharToValue(command_buffer[1]);
    if (command_buffer[2] != '\0')
    {
        id <<= 4;
        id |= HexCharToValue(command_buffer[2]);
    }

    std::cout << "read register: " << std::to_string(id) << std::endl;

    LongToGdbHex(reply, RegRead(id));

    gdbstub_sendreply(reinterpret_cast<char*>(reply));
}

/// Send all registers to the gdb client.
static void ReadRegisters()
{
    static uint8_t buffer[GDB_BUFFER_SIZE - 4];
    memset(buffer, 0, sizeof(buffer));

    uint8_t* bufptr = buffer;

    std::cout << "read registers " << std::endl;

    std::cout << fmt_string("PC: {:08X}", RegRead(PC_REGISTER)) << std::endl;

    for (uint32_t reg = 0; reg <= FIR_REGISTER; reg++)
    {
        LongToGdbHex(bufptr + reg * 16, RegRead(reg));
    }

    gdbstub_sendreply(reinterpret_cast<char*>(buffer));
}

/// Modify data of register specified by gdb client.
static void WriteRegister()
{
    const uint8_t* buffer_ptr = command_buffer + 3;

    uint32_t id = HexCharToValue(command_buffer[1]);
    if (command_buffer[2] != '=') {
        ++buffer_ptr;
        id <<= 4;
        id |= HexCharToValue(command_buffer[2]);
    }

    RegWrite(id, GdbHexToLong(buffer_ptr));

    gdbstub_sendreply("OK");
}

/// Modify all registers with data received from the client.
static void WriteRegisters()
{
    const uint8_t* buffer_ptr = command_buffer + 1;

    if (command_buffer[0] != 'G')
        return gdbstub_sendreply("E01");

    for (uint32_t i = 0, reg = 0; reg <= FIR_REGISTER; i++, reg++) {
        RegWrite(reg, GdbHexToInt(buffer_ptr + i * 8));
    }

    gdbstub_sendreply("OK");
}

/// Read location in memory specified by gdb client.
static void ReadMemory()
{
    static uint8_t reply[GDB_BUFFER_SIZE - 4];

    auto start_offset = command_buffer + 1;
    const auto addr_pos = std::find(start_offset, command_buffer + command_length, ',');
    const uint64_t addr = HexToLong(start_offset, static_cast<uint64_t>(addr_pos - start_offset));

    start_offset = addr_pos + 1;
    const uint64_t len =
        HexToLong(start_offset, static_cast<uint64_t>((command_buffer + command_length) - start_offset));

    LOG_DEBUG("gdb: addr: {:016X} len: {:08X}", addr, len);

    if (len * 2 > sizeof(reply))
    {
        gdbstub_sendreply("E01");
    }

    // TODO: see why this fails sometimes
    // in mupen64plus-core and fix it upstream
    /*if(!(m64p::Debug.MemGetMemInfo(M64P_DBG_MEM_FLAGS, (uint32_t)addr) & M64P_MEM_FLAG_READABLE))
    {
        return gdbstub_sendreply("E00");
    }*/

    std::vector<uint8_t> data(len);
    for (uint32_t i = 0; i < len; i++)
        data[i] = m64p::Debug.MemRead8((uint32_t)(addr + i));

    MemToGdbHex(reply, data.data(), len);
    reply[len * 2] = '\0';
    gdbstub_sendreply(reinterpret_cast<char*>(reply));
}

/// Modify location in memory with data received from the gdb client.
static void WriteMemory()
{
    auto start_offset = command_buffer + 1;
    auto addr_pos = std::find(start_offset, command_buffer + command_length, ',');
    uint32_t addr = HexToInt(start_offset, static_cast<uint32_t>(addr_pos - start_offset));

    start_offset = addr_pos + 1;
    auto len_pos = std::find(start_offset, command_buffer + command_length, ':');
    uint32_t len = HexToInt(start_offset, static_cast<uint32_t>(len_pos - start_offset));

    if((m64p::Debug.MemGetMemInfo(M64P_DBG_MEM_FLAGS, addr) & M64P_MEM_FLAG_WRITABLE) == 0)
    {
        return gdbstub_sendreply("E00");
    }

    std::vector<uint8_t> data(len);

    GdbHexToMem(data.data(), len_pos + 1, len);
    for (uint32_t i = 0; i < len; i++)
    {
        m64p::Debug.MemWrite8((uint32_t)(addr + i), data[i]);
    }
    gdbstub_sendreply("OK");
}

/// Tell the CPU that it should perform a single step.
static void Step()
{
    std::cout << "GDBStub Step()" << std::endl;
    if (command_length > 1)
    {
        RegWrite(PC_REGISTER, GdbHexToLong(command_buffer + 1));
    }
    m64p::Debug.Step();
}

/// Tell the CPU to continue executing.
static void Continue()
{
    m64p::Debug.SetRunState(M64P_DBG_RUNSTATE_RUNNING);
}

/**
 * Commit breakpoint to list of breakpoints.
 *
 * @param type Type of breakpoint.
 * @param addr Address of breakpoint.
 * @param len Length of breakpoint.
 */
static bool CommitBreakpoint(uint8_t type, uint32_t addr, uint32_t len)
{
    m64p_breakpoint bkpt;
    bkpt.address = addr;
    bkpt.endaddr = (addr + len);
    bkpt.flags = M64P_BKP_FLAG_ENABLED | M64P_BKP_FLAG_LOG;

    switch (type) {
    case 0:
    case 1:
        bkpt.flags |= M64P_BKP_FLAG_EXEC;
        break;
    case 2:
        bkpt.flags |= M64P_BKP_FLAG_WRITE;
        break;
    case 3:
        bkpt.flags |= M64P_BKP_FLAG_READ;
        break;
    case 4:
        bkpt.flags |= M64P_BKP_FLAG_WRITE | M64P_BKP_FLAG_READ;
        break;
    default:
        return false;
    }

    if (m64p::Debug.BreakpointLookup(bkpt.address, (uint32_t)len, bkpt.flags) >= 0)
    {
        LOG_ERROR("Trying to add already existing breakpoint");
        return false;
    }

    int numBkps = m64p::Debug.BreakpointCommand(M64P_BKP_CMD_ADD_STRUCT, 0, &bkpt);
    if (numBkps == -1)
    {
        LOG_ERROR("Maximum breakpoint limit already reached.\n");
        return false;
    }

    LOG_DEBUG("gdb: added {} breakpoint: {:016X} bytes at {:016X}",
              static_cast<int>(type), len, addr);

    return true;
}

/// Handle add breakpoint command from gdb client.
static void AddBreakpoint()
{
    uint8_t type_id = HexCharToValue(command_buffer[1]);

    if (type_id > 4)
    {
        gdbstub_sendreply("E01");
        return;
    }

    auto start_offset = command_buffer + 3;
    auto addr_pos = std::find(start_offset, command_buffer + command_length, ',');
    uint32_t addr = HexToInt(start_offset, static_cast<uint32_t>(addr_pos - start_offset));

    start_offset = addr_pos + 1;
    uint32_t len = HexToInt(start_offset, static_cast<uint32_t>((command_buffer + command_length) - start_offset));

    if (!CommitBreakpoint(type_id, addr, len))
    {
        gdbstub_sendreply("E02");
    }
    else
    {
        gdbstub_sendreply("OK");
    }
}

/// Handle remove breakpoint command from gdb client.
static void RemoveBreakpoint()
{
    uint8_t type_id = HexCharToValue(command_buffer[1]);

    if (type_id > 4)
    {
        gdbstub_sendreply("E01");
        return;
    }

    auto start_offset = command_buffer + 3;
    auto addr_pos = std::find(start_offset, command_buffer + command_length, ',');
    uint32_t addr = HexToInt(start_offset, static_cast<uint32_t>(addr_pos - start_offset));

    if(!RemoveBreakpoint(type_id, addr))
    {
        gdbstub_sendreply("E02");
    }
    else
    {
        gdbstub_sendreply("OK");
    }
}

void gdbstub_handle_packet(bool check)
{
    if (!first_exception && !check)
    {
        SendSignal(SIGTRAP);
    }

    first_exception = false;

    while (1)
    {
        if (gdbserver_socket < 0)
            return;

        if (!IsDataAvailable())
        {
            if (check)
            {
              return;
            }
            else
            {
              continue;
            }
        }

        ReadCommand();
        if (command_length == 0)
        {
            continue;
        }

        LOG_DEBUG("Packet: {}", (char*)command_buffer);

        switch (command_buffer[0]) {
        case 'q':
            HandleQuery();
            break;
        case 'H':
            HandleSetThread();
            break;
        case '!':
            gdbstub_sendreply("OK");
            break;
        case '?':
            SendSignal(latest_signal);
            break;
        case 'k':
            CoreGDBStubShutdown();
            LOG_INFO("killed by gdb");
            return;
        case 'g':
            ReadRegisters();
            break;
        case 'G':
            WriteRegisters();
            break;
        case 'p':
            ReadRegister();
            break;
        case 'P':
            WriteRegister();
            break;
        case 'm':
            ReadMemory();
            break;
        case 'M':
            WriteMemory();
            break;
        case 's':
            Step();
            return;
        case 'C':
        case 'c':
            Continue();
            return;
        case 'z':
            RemoveBreakpoint();
            break;
        case 'Z':
            AddBreakpoint();
            break;
        case 'T':
            HandleThreadAlive();
            break;
        default:
            gdbstub_sendreply("");
            break;
        }
    }
}

static void debugger_init(void)
{
    LOG_DEBUG("Debugger initialized.");
}

static void debugger_update(unsigned int pc)
{
    LOG_DEBUG("PC at {:08X}.", pc);
    gdbstub_handle_packet(false);
}

static void debugger_vi(void)
{
    gdbstub_handle_packet(true);
}

//
// Exported Functions
//

bool CoreGDBStubInit(int port)
{
    m64p::Debug.SetCallbacks(debugger_init, debugger_update, debugger_vi);

    // Start gdb server
    LOG_INFO("Starting GDB server on port {}...", port);

    sockaddr_in saddr_server = {};
    saddr_server.sin_family = AF_INET;
    saddr_server.sin_port = htons(port);
    saddr_server.sin_addr.s_addr = INADDR_ANY;

#ifdef _WIN32
    WSAStartup(MAKEWORD(2, 2), &InitData);
#endif

    int tmpsock = static_cast<int>(socket(PF_INET, SOCK_STREAM, 0));
    if (tmpsock == -1)
    {
        LOG_ERROR("Failed to create gdb socket");
        return false;
    }

    // Set socket to SO_REUSEADDR so it can always bind on the same port
    int reuse_enabled = 1;
    if (setsockopt(tmpsock, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse_enabled,
        sizeof(reuse_enabled)) < 0)
    {
        LOG_ERROR("Failed to set gdb socket option");
        return false;
    }

    const sockaddr* server_addr = reinterpret_cast<const sockaddr*>(&saddr_server);
    socklen_t server_addrlen = sizeof(saddr_server);
    if (bind(tmpsock, server_addr, server_addrlen) < 0)
    {
        LOG_ERROR("Failed to bind gdb socket");
        return false;
    }

    if (listen(tmpsock, 1) < 0)
    {
        LOG_ERROR("Failed to listen to gdb socket");
        return false;
    }

    // Wait for gdb to connect
    LOG_INFO("Waiting for gdb to connect...");
    sockaddr_in saddr_client;
    sockaddr* client_addr = reinterpret_cast<sockaddr*>(&saddr_client);
    socklen_t client_addrlen = sizeof(saddr_client);
    gdbserver_socket = static_cast<int>(accept(tmpsock, client_addr, &client_addrlen));
    if (gdbserver_socket < 0)
    {
        LOG_ERROR("Failed to accept gdb client");
        return false;
    }
    else
    {
        LOG_INFO("Client connected.");
        saddr_client.sin_addr.s_addr = ntohl(saddr_client.sin_addr.s_addr);
    }

    // Clean up temporary socket if it's still alive at this point.
    if (tmpsock != -1)
    {
        shutdown(tmpsock, SHUT_RDWR);
    }
    return true;
}

void CoreGDBStubShutdown()
{
    LOG_INFO("Stopping GDB ...");

    int numBkpts = m64p::Debug.GetState(M64P_DBG_NUM_BREAKPOINTS);

    for(int i = 0; i < numBkpts; i++)
    {
        m64p::Debug.BreakpointCommand(M64P_BKP_CMD_REMOVE_IDX, i, nullptr);
    }

    m64p::Debug.SetRunState(M64P_DBG_RUNSTATE_RUNNING);
    m64p::Debug.SetCallbacks(nullptr, nullptr, nullptr);

    if (gdbserver_socket != -1)
    {
        shutdown(gdbserver_socket, SHUT_RDWR);
        gdbserver_socket = -1;
    }

#ifdef _WIN32
    WSACleanup();
#endif

    LOG_INFO("GDB stopped.");
}
