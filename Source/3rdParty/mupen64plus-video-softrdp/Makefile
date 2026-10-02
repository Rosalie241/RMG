# Host detection. Windows_NT is set by cmd.exe for every Windows version, not
# just NT; anything else is assumed POSIX. This decides the recipe shell syntax
# and the default compiler. What gets built is decided by CC's target below.
ifeq ($(OS),Windows_NT)
WINDOWS := 1
else
WINDOWS := 0
endif

# Only override a compiler make invented for itself; CC from the environment or
# the command line always wins.
ifeq ($(origin CC),default)
ifeq ($(WINDOWS),1)
CC := gcc
else
# make's built-in default is a bare "cc", which on a distro that ships clang
# without gcc may not exist at all - and hardcoding gcc has the same problem in
# reverse. Ask the system which one is actually there.
CC := $(shell command -v clang 2>/dev/null || command -v cc 2>/dev/null || echo gcc)
endif
endif

ifeq ($(WINDOWS),1)
ifeq ($(origin RM),default)
RM := del /Q
endif
# $(call native,path) - backslashes for cmd.exe, which rejects forward slashes
# in the argument position of its builtins.
native = $(subst /,\,$1)
MKDIR = if not exist $(call native,$1) mkdir $(call native,$1)
else
native = $1
MKDIR = mkdir -p $1
endif

# 32-bit MinGW cross toolchain (Project64 target)
PJ64_CC  ?= i686-w64-mingw32-gcc

# Target detection. CC may be a MinGW cross compiler on a POSIX host (the
# release workflow builds both DLLs from Linux), so which platform sources
# compile and the artefact suffix follow CC's target, not the host. Clang may
# canonicalize x86_64-w64-mingw32 to x86_64-w64-windows-gnu; accept both.
CC_MACHINE := $(shell $(CC) -dumpmachine)
ifeq ($(WINDOWS),1)
PJ64_CC_MACHINE := $(shell $(PJ64_CC) -dumpmachine)
endif
ifneq (,$(or $(findstring mingw,$(CC_MACHINE)),$(findstring windows,$(CC_MACHINE))))
TARGET_WINDOWS := 1
DLL := .dll
else
TARGET_WINDOWS := 0
DLL := .so
endif

# build   = 64-bit objects + mupen64plus DLL
# build32 = 32-bit objects + Project64 DLL
BUILD_DIR   := build
BUILD32_DIR := build32

# A Windows checkout and a WSL build of it are the same directory on the same
# disk, so the two hosts must not share an object directory: make would see the
# other host's objects as up to date and link a foreign object format - or, with
# LTO, bitcode from a different toolchain. Only the non-Windows path is keyed,
# which leaves every existing Windows path exactly where it was.
ifeq ($(WINDOWS),1)
OBJ_DIR := $(BUILD_DIR)
else
UNAME_S := $(shell uname -s)
OBJ_DIR := $(BUILD_DIR)/$(shell echo $(UNAME_S) | tr '[:upper:]' '[:lower:]')
ifeq ($(TARGET_WINDOWS),1)
OBJ_DIR := $(OBJ_DIR)-$(CC_MACHINE)
endif
endif

# ---- Flags -----------------------------------------------------------------
CFLAGS ?= -O3 -g -flto

# Development knobs. All three are off in a shipping build and none of them is
# read anywhere in the render path, so leaving them alone costs nothing.
SOFTRDP_LOG ?= 0

# Pixel census (src/core/kernel_census.h): counts pixels per draw state and
# prints the tables at exit.
SOFTRDP_CENSUS ?= 0

# Frame dump recorder: the file format writer (src/core/sr_dump.c) and the host
# driver that feeds it (src/plugin/plugin_dump.c). Neither source is compiled
# unless this is set; plugin_dump.h supplies no-op inlines when it is not.
SOFTRDP_DUMP ?= 0

CFLAGS += -std=c17 -Wall -Wextra -Wpedantic -MMD -MP -Isrc \
	-DSOFTRDP_ENABLE_LOG=$(SOFTRDP_LOG) \
	-DSOFTRDP_CENSUS=$(SOFTRDP_CENSUS) \
	-DSOFTRDP_ENABLE_DUMP=$(SOFTRDP_DUMP)

# Enable AVX2 for x86 compiler targets without imposing an x86-only flag on
# native ARM and other hosts. Detect the compiler target, not the machine
# running make: PJ64_CC is normally an i686 cross compiler even on an ARM host.
X86_CFLAGS ?= -mavx2
X86_TARGET_PATTERNS := i386% i486% i586% i686% x86_64% amd64%
HOST_ARCH_CFLAGS := $(if $(filter $(X86_TARGET_PATTERNS),$(CC_MACHINE)),$(X86_CFLAGS))
PJ64_ARCH_CFLAGS := $(if $(filter $(X86_TARGET_PATTERNS),$(PJ64_CC_MACHINE)),$(X86_CFLAGS))
HOST_CFLAGS = $(CFLAGS) $(HOST_ARCH_CFLAGS)
PJ64_CFLAGS = $(CFLAGS) $(PJ64_ARCH_CFLAGS)

# One core copy per scale: the scale as a literal, and every exported symbol
# renamed so both copies can be linked into one binary (see sr_namespace.h).
SCALE1_FLAGS := -DSOFTRDP_SCALE=1 -DSR_SCALE_SUFFIX=_s1 -include src/core/sr_namespace.h
SCALE2_FLAGS := -DSOFTRDP_SCALE=2 -DSR_SCALE_SUFFIX=_s2 -include src/core/sr_namespace.h

LDFLAGS ?= -flto

# -flto leaves LLVM bitcode in the objects, and binutils' bfd ld only reads that
# through the LLVMgold plugin, which ships in a separate llvm-*-dev package. lld
# reads it natively and comes with clang, so prefer it rather than making the
# build depend on a package that may not be installed.
#
# Two hosts must be left alone. Apple's clang drives ld64, which reads bitcode
# through libLTO natively and rejects -fuse-ld=lld with "invalid linker name";
# and a clang installed without lld fails the same way. So ask for lld only
# where it is both needed and actually present.
ifneq (,$(findstring clang,$(CC)))
ifneq ($(UNAME_S),Darwin)
ifneq (,$(shell command -v ld.lld 2>/dev/null))
LDFLAGS += -fuse-ld=lld
endif
endif
endif

# Platform layer: the only core sources tied to an OS. A port swaps this list
# for its own <name>_<platform>.c files and leaves the other lists untouched.
ifeq ($(TARGET_WINDOWS),1)
PLATFORM_SRCS := \
	src/core/sr_threads_win32.c
PLUGIN_LIBS := -lopengl32 -lgdi32 -luser32
else
PLATFORM_SRCS := \
	src/core/sr_threads_posix.c
# No -lGL: every GL entry point is resolved through the host's proc loader or
# dlopen'd from libGL.so.1 at runtime, so the build needs neither the GL headers
# nor the libGL.so symlink that only the development package installs. See
# src/present/sr_gl.h. -ldl carries both that and the plugin's own dlsym calls.
PLUGIN_LIBS := -lpthread -ldl
# Shared objects must be position independent; the Windows toolchain does this
# for DLLs implicitly, ELF does not.
CFLAGS += -fPIC
endif

# Compiled once per scale, then namespaced. SOFTRDP_SCALE is deliberately not
# defined anywhere else in this file: sr_scale.h defaults it to 1, which is what
# the shared and plugin translation units want, and only these two rules
# override it.
SCALED_SRCS := \
	src/core/sr.c \
	src/core/rdp_commands.c \
	src/core/rdp_memory.c \
	src/core/framebuffer.c \
	src/core/raster.c \
	src/core/tmem.c \
	src/core/primitive.c \
	src/core/kernel.c \
	src/core/blend_divider.c \
	src/core/vi.c

# Compiled once, shared by both copies: the scale selector (the public sr_* API)
# and the thread pool, which is process-wide and has no scale dependency.
SHARED_CORE_SRCS := \
	src/core/sr_dispatch.c \
	src/core/sr_threads_vi.c \
	src/core/sr_async.c \
	$(PLATFORM_SRCS)

ifeq ($(SOFTRDP_DUMP),1)
SHARED_CORE_SRCS += src/core/sr_dump.c
endif

# What both host adapters share once the host ABI has been unpacked: context
# lifetime, the frame path, and the dump hooks. Linked into both DLLs.
PLUGIN_SHARED_SRCS := \
	src/plugin/plugin_common.c

ifeq ($(SOFTRDP_DUMP),1)
PLUGIN_SHARED_SRCS += src/plugin/plugin_dump.c
endif

PJ64_SRCS := \
	src/plugin/pj64/pj64_gfx.c \
	src/plugin/pj64/pj64_log.c

M64P_SRCS := \
	src/plugin/mupen64plus/mupen64plus_gfx.c

# One presenter for every OS. Its owned-context (WGL) half compiles away off
# Windows; mupen64plus only ever uses the external-context half.
PRESENT_SRCS := \
	src/present/sr_present_gl.c \
	src/present/sr_field_output.c

# ---- Objects ---------------------------------------------------------------
# 64-bit set (mupen64plus): core + present + plugin-shared + m64p
CORE_OBJS    := $(patsubst src/core/%.c,$(OBJ_DIR)/core_s1_%.o,$(SCALED_SRCS)) \
	$(patsubst src/core/%.c,$(OBJ_DIR)/core_s2_%.o,$(SCALED_SRCS)) \
	$(patsubst src/core/%.c,$(OBJ_DIR)/core_%.o,$(SHARED_CORE_SRCS))
PRESENT_OBJS := $(patsubst src/present/%.c,$(OBJ_DIR)/present_%.o,$(PRESENT_SRCS))
SHARED_OBJS  := $(patsubst src/plugin/%.c,$(OBJ_DIR)/plugin_%.o,$(PLUGIN_SHARED_SRCS))
M64P_OBJS    := $(patsubst src/plugin/mupen64plus/%.c,$(OBJ_DIR)/m64p_%.o,$(M64P_SRCS))

# 32-bit set (Project64): core + present + plugin-shared + pj64
CORE32_OBJS    := $(patsubst src/core/%.c,$(BUILD32_DIR)/core_s1_%.o,$(SCALED_SRCS)) \
	$(patsubst src/core/%.c,$(BUILD32_DIR)/core_s2_%.o,$(SCALED_SRCS)) \
	$(patsubst src/core/%.c,$(BUILD32_DIR)/core_%.o,$(SHARED_CORE_SRCS))
PRESENT32_OBJS := $(patsubst src/present/%.c,$(BUILD32_DIR)/present_%.o,$(PRESENT_SRCS))
SHARED32_OBJS  := $(patsubst src/plugin/%.c,$(BUILD32_DIR)/plugin_%.o,$(PLUGIN_SHARED_SRCS))
PJ64_OBJS      := $(patsubst src/plugin/pj64/%.c,$(BUILD32_DIR)/pj64_%.o,$(PJ64_SRCS))

PJ64_DLL := $(BUILD32_DIR)/softrdp-pj64.dll
# A Windows DLL always lands in build/, next to where a Windows host puts it.
ifeq ($(TARGET_WINDOWS),1)
M64P_DLL := $(BUILD_DIR)/mupen64plus-video-softrdp$(DLL)
else
M64P_DLL := $(OBJ_DIR)/mupen64plus-video-softrdp$(DLL)
endif

# ---- Top-level targets -----------------------------------------------------
.PHONY: all pj64 mupen64plus clean print-config

# The Project64 plugin is a 32-bit Windows DLL by definition, so unless CC
# targets Windows "all" is the mupen64plus plugin alone.
ifeq ($(TARGET_WINDOWS),1)
all: pj64 mupen64plus
else
all: mupen64plus
endif
pj64: $(PJ64_DLL)
mupen64plus: $(M64P_DLL)

print-config:
	@echo WINDOWS=$(WINDOWS) TARGET_WINDOWS=$(TARGET_WINDOWS)
	@echo CC=$(CC) CC_MACHINE=$(CC_MACHINE)
	@echo PJ64_CC=$(PJ64_CC) PJ64_CC_MACHINE=$(PJ64_CC_MACHINE)
	@echo OBJ_DIR=$(OBJ_DIR) M64P_DLL=$(M64P_DLL) PJ64_DLL=$(PJ64_DLL)

# ---- Directory creation (order-only, never triggers a rebuild) -------------
# sort dedupes: OBJ_DIR is BUILD_DIR on Windows.
$(sort $(BUILD_DIR) $(BUILD32_DIR) $(OBJ_DIR)):
	@$(call MKDIR,$@)

# ---- Compile: 64-bit (host) ------------------------------------------------
# Make prefers the shortest stem, so core_s1_sr.o takes the s1 rule below
# rather than core_%.o with the stem "s1_sr".
$(OBJ_DIR)/core_s1_%.o: src/core/%.c | $(OBJ_DIR)
	$(CC) $(HOST_CFLAGS) $(SCALE1_FLAGS) -c $< -o $@
$(OBJ_DIR)/core_s2_%.o: src/core/%.c | $(OBJ_DIR)
	$(CC) $(HOST_CFLAGS) $(SCALE2_FLAGS) -c $< -o $@
$(OBJ_DIR)/core_%.o: src/core/%.c | $(OBJ_DIR)
	$(CC) $(HOST_CFLAGS) -c $< -o $@

$(OBJ_DIR)/m64p_%.o: src/plugin/mupen64plus/%.c | $(OBJ_DIR)
	$(CC) $(HOST_CFLAGS) -c $< -o $@
$(OBJ_DIR)/plugin_%.o: src/plugin/%.c | $(OBJ_DIR)
	$(CC) $(HOST_CFLAGS) -c $< -o $@
$(OBJ_DIR)/present_%.o: src/present/%.c | $(OBJ_DIR)
	$(CC) $(HOST_CFLAGS) -c $< -o $@

# ---- Compile: 32-bit (MinGW cross) -----------------------------------------
$(BUILD32_DIR)/core_s1_%.o: src/core/%.c | $(BUILD32_DIR)
	$(PJ64_CC) $(PJ64_CFLAGS) $(SCALE1_FLAGS) -c $< -o $@
$(BUILD32_DIR)/core_s2_%.o: src/core/%.c | $(BUILD32_DIR)
	$(PJ64_CC) $(PJ64_CFLAGS) $(SCALE2_FLAGS) -c $< -o $@
$(BUILD32_DIR)/core_%.o: src/core/%.c | $(BUILD32_DIR)
	$(PJ64_CC) $(PJ64_CFLAGS) -c $< -o $@

$(BUILD32_DIR)/pj64_%.o: src/plugin/pj64/%.c | $(BUILD32_DIR)
	$(PJ64_CC) $(PJ64_CFLAGS) -c $< -o $@
$(BUILD32_DIR)/plugin_%.o: src/plugin/%.c | $(BUILD32_DIR)
	$(PJ64_CC) $(PJ64_CFLAGS) -c $< -o $@
$(BUILD32_DIR)/present_%.o: src/present/%.c | $(BUILD32_DIR)
	$(PJ64_CC) $(PJ64_CFLAGS) -c $< -o $@

# ---- Link ------------------------------------------------------------------
# The shipped plugins drop their debug sections: -g stays on every object, so
# the build tree is still fully debuggable, but the DWARF it produces is ~85% of
# an unstripped DLL (16.6 of 20.1 MB) and is never loaded at run time.
# `make PLUGIN_LDFLAGS=` keeps it.
#
# Not on macOS: ld64 leaves DWARF in the .o files and only ever collects it into
# a .dSYM bundle on demand, so the linked library is already free of it and has
# nothing to strip - and ld64 treats -s as obsolete.
ifeq ($(UNAME_S),Darwin)
PLUGIN_LDFLAGS ?=
else
PLUGIN_LDFLAGS ?= -s
endif

$(M64P_DLL): $(CORE_OBJS) $(PRESENT_OBJS) $(SHARED_OBJS) $(M64P_OBJS)
	$(CC) -shared $^ -o $@ $(LDFLAGS) $(PLUGIN_LDFLAGS) $(PLUGIN_LIBS)

$(PJ64_DLL): $(CORE32_OBJS) $(PRESENT32_OBJS) $(SHARED32_OBJS) $(PJ64_OBJS)
	$(PJ64_CC) -shared $^ -o $@ $(LDFLAGS) $(PLUGIN_LDFLAGS) $(PLUGIN_LIBS)

# ---- Clean -----------------------------------------------------------------
ifeq ($(WINDOWS),1)
clean:
	@if exist $(BUILD_DIR) $(RM) $(BUILD_DIR)\*.o $(BUILD_DIR)\*.d $(BUILD_DIR)\*.dll
	@if exist $(BUILD32_DIR) $(RM) $(BUILD32_DIR)\*.o $(BUILD32_DIR)\*.d $(BUILD32_DIR)\*.dll
else
clean:
	@rm -f $(OBJ_DIR)/*.o $(OBJ_DIR)/*.d $(OBJ_DIR)/*$(DLL) $(M64P_DLL)
	@rm -f $(BUILD32_DIR)/*.o $(BUILD32_DIR)/*.d $(BUILD32_DIR)/*.dll
endif

# ---- Auto-generated header dependencies ------------------------------------
# This is what makes incremental builds correct: touching a header now
# recompiles exactly the translation units that include it (and nothing else),
# instead of forcing a full clean rebuild.
DEPS := $(wildcard $(OBJ_DIR)/*.d $(BUILD32_DIR)/*.d)
-include $(DEPS)
