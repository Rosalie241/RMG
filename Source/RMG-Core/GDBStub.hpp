// Taken from https://github.com/Gillou68310/mupen64plus-ui-console/blob/gdbstub/src/gdbstub.h
// And modified for RMG-Core

// Copyright 2013 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

// Originally written by Sven Peter <sven@fail0verflow.com> for anergistic.

#ifndef CORE_GDBSTUB_H
#define CORE_GDBSTUB_H

// attempts to initialize GDBStub
bool CoreGDBStubInit(int port);

// shuts down GDB stub
void CoreGDBStubShutdown(void);

#endif // CORE_GDBSTUB_H
