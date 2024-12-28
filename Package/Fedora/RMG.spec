Name:    RMG       
Version: 0.6.8
Release: %autorelease
Summary: Rosalie's Mupen GUI 

License: GPL-3.0-only       
URL:     https://github.com/Rosalie241/RMG
Source0: https://github.com/Rosalie241/RMG/archive/refs/tags/v%{version}.tar.gz       

BuildRequires: gcc
BuildRequires: g++
BuildRequires: nasm
BuildRequires: cmake
BuildRequires: hidapi-devel
BuildRequires: libsamplerate-devel
BuildRequires: minizip-compat-devel
BuildRequires: SDL2-devel
BuildRequires: SDL2_net-devel
BuildRequires: freetype-devel
BuildRequires: mesa-libGL-devel
BuildRequires: mesa-libGLU-devel
BuildRequires: zlib-ng-devel
BuildRequires: binutils-devel
BuildRequires: speexdsp-devel
BuildRequires: qt6-qtbase-devel
BuildRequires: qt6-qtsvg-devel
BuildRequires: qt6-qtwebsockets-devel
BuildRequires: libxkbcommon-devel

Requires: hidapi
Requires: SDL2
Requires: SDL2_net
Requires: zlib-ng
Requires: libsamplerate
Requires: speexdsp
Requires: qt6-qtbase
Requires: qt6-qtsvg
Requires: qt6-qtwebsockets

%description
Rosalie's Mupen GUI is a free and open-source mupen64plus front-end written in C++

%prep
%autosetup

%build
%cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DPORTABLE_INSTALL=OFF -DDISCORD_RPC=OFF
%cmake_build

%install
%cmake_install

%files
/usr/bin/RMG
/usr/lib64/libRMG-Core.so
/usr/lib64/RMG/
/usr/share/RMG/
/usr/share/applications/com.github.Rosalie241.RMG.desktop
/usr/share/icons/hicolor/scalable/apps/com.github.Rosalie241.RMG.svg
/usr/share/metainfo/com.github.Rosalie241.RMG.metainfo.xml

%changelog
%autochangelog
