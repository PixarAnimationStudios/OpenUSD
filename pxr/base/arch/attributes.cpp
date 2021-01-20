// Copyright 2016 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//

#include "pxr/pxr.h"
#include "pxr/base/arch/attributes.h"
#include "pxr/base/arch/error.h"

#if defined(ARCH_OS_DARWIN)

#include <mach-o/dyld.h>
#include <mach-o/loader.h>
#include <mach-o/swap.h>
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

namespace {

// Minimal access to a Mach-O header, providing just enough to find a
// named section in a named segment.  This assumes the headers have
// been mapped by dyld and are therefore already in host byte order.
class MachO {
public:
    struct SectionInfo {
        intptr_t address;
        ptrdiff_t size;
    };

    MachO(const struct mach_header* mh, intptr_t slide);

    // Returns the address and size of the section named sectname in
    // the segment named segname.  Returns a zero size and address if
    // not found.
    SectionInfo find(const char* segname, const char* sectname) const;

private:
    template <class T>
    const T* get(size_t offset) const
    {
        return reinterpret_cast<const T*>(get2(offset));
    }

    const void* get2(size_t offset) const
    {
        return reinterpret_cast<const uint8_t*>(_mh) + offset;
    }

private:
    const struct mach_header* _mh;
    intptr_t _slide;
    size_t _ncmds;
    size_t _cmdsOffset;
};

MachO::MachO(const struct mach_header* mh, intptr_t slide) :
    _mh(mh),
    _slide(slide)
{
    if (_mh->magic == MH_MAGIC_64) {
        auto header = get<struct mach_header_64>(0);
        _ncmds = header->ncmds;
        _cmdsOffset = sizeof(struct mach_header_64);
    }
    else if (_mh->magic == MH_MAGIC) {
        auto header = get<struct mach_header>(0);
        _ncmds = header->ncmds;
        _cmdsOffset = sizeof(struct mach_header);
    }
    else if (_mh->magic == MH_CIGAM_64 || _mh->magic == MH_CIGAM) {
        ARCH_ERROR("Loaded byte-swapped MachO object");
    }
    else {
        ARCH_ERROR("Unrecognized MachO object");
    }
}

MachO::SectionInfo
MachO::find(const char* segname, const char* sectname) const
{
    bool is_64 = false;
    size_t nsects = 0;
    size_t offset = _cmdsOffset;
    for (size_t i = 0; i != _ncmds; ++i) {
        auto cmd = get<struct load_command>(offset);
        if (cmd->cmd == LC_SEGMENT_64) {
            auto segment = get<struct segment_command_64>(offset);
            if (strcmp(segment->segname, segname) == 0) {
                nsects = segment->nsects;
                offset += sizeof(struct segment_command_64);
                is_64 = true;
                break;
            }
        }
        else if (cmd->cmd == LC_SEGMENT) {
            auto segment = get<struct segment_command>(offset);
            if (strcmp(segment->segname, segname) == 0) {
                nsects = segment->nsects;
                offset += sizeof(struct segment_command);
                break;
            }
        }
        offset += cmd->cmdsize;
    }

    if (is_64) {
        for (size_t i = 0; i != nsects; ++i) {
            auto section = get<struct section_64>(offset);
            if (strcmp(section->sectname, sectname) == 0) {
                return { static_cast<intptr_t>(section->addr) + _slide,
                         static_cast<ptrdiff_t>(section->size) };
            }
            offset += sizeof(*section);
        }
    }
    else {
        for (size_t i = 0; i != nsects; ++i) {
            auto section = get<struct section>(offset);
            if (strcmp(section->sectname, sectname) == 0) {
                return { static_cast<intptr_t>(section->addr) + _slide,
                         static_cast<ptrdiff_t>(section->size) };
            }
            offset += sizeof(*section);
        }
    }

    return { 0, 0 };
}

static
std::vector<Arch_ConstructorEntry>
GetConstructorEntries(
    const struct mach_header* mh, intptr_t slide,
    const char* segname, const char* sectname)
{
    std::vector<Arch_ConstructorEntry> result;

    // Find the section.
    const auto info = MachO(mh, slide).find(segname, sectname);

    // Done if not found or empty.
    const ptrdiff_t numEntries = info.size / sizeof(Arch_ConstructorEntry);
    if (numEntries == 0) {
        return result;
    }

    // Copy the entries for sorting.  We could sort in place but we want
    // to return a vector anyway.
    const Arch_ConstructorEntry* entries =
        reinterpret_cast<const Arch_ConstructorEntry*>(info.address);
    result.assign(entries, entries + numEntries);

    // Sort.
    std::sort(result.begin(), result.end(),
        [](const Arch_ConstructorEntry& lhs, const Arch_ConstructorEntry& rhs){
            return lhs.priority < rhs.priority;
        });

    return result;
}

// Execute constructor entries in a shared library in priority order.
static
void
AddImage(const struct mach_header* mh, intptr_t slide)
{
    const auto entries = GetConstructorEntries(mh, slide, "__DATA", "pxrctor");

    // Execute in priority order.
    for (size_t i = 0, n = entries.size(); i != n; ++i) {
        if (entries[i].function && entries[i].version == 0u) {
            entries[i].function();
        }
    }
}

// Execute destructor entries in a shared library in reverse priority order.
static
void
RemoveImage(const struct mach_header* mh, intptr_t slide)
{
    const auto entries = GetConstructorEntries(mh, slide, "__DATA", "pxrdtor");

    // Execute in reverse priority order.
    for (size_t i = entries.size(); i-- != 0; ) {
        if (entries[i].function && entries[i].version == 0u) {
            entries[i].function();
        }
    }
}

// Make sure our callbacks are installed when this library is loaded.
// The registration functions will call the callbacks for all currently
// loaded images so we can't possibly miss any though we don't know what
// order they'll run in.  That shouldn't matter since anything that uses
// ARCH_CONSTRUCTOR/ARCH_DESTRUCTOR should load this library first so
// any already-loaded images shouldn't have any constructors/destructors
// to call.
__attribute__((used, constructor)) \
static void InstallDyldCallbacks()
{
    _dyld_register_func_for_add_image(AddImage);
    _dyld_register_func_for_remove_image(RemoveImage);
}

} // anonymous namespace

PXR_NAMESPACE_CLOSE_SCOPE

#elif defined(ARCH_OS_WINDOWS)

#include <Windows.h>
#include <algorithm>
#include <cstddef>
#include <cstring>
#include <set>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

namespace {

// Minimal access to an NT header, providing just enough to find a
// named section.  This assumes the headers have been mapped into the
// process.
class ImageNT {
public:
    struct SectionInfo {
        intptr_t address;
        ptrdiff_t size;
    };

    ImageNT(HMODULE hModule) : _hModule(hModule) { }

    // Returns the address and size of the section named sectname.
    // Returns a zero size and address if not found.
    SectionInfo Find(const char* sectname) const;

private:
    HMODULE _hModule;
};

ImageNT::SectionInfo
ImageNT::Find(const char* sectname) const
{
    // Get the section headers and the number of sections.
    intptr_t base = reinterpret_cast<intptr_t>(_hModule);
    const IMAGE_DOS_HEADER* dosHeader =
        reinterpret_cast<IMAGE_DOS_HEADER*>(base);
    const IMAGE_NT_HEADERS* ntHeader =
        reinterpret_cast<IMAGE_NT_HEADERS*>(base + dosHeader->e_lfanew);
    const IMAGE_SECTION_HEADER* sectionHeaders = IMAGE_FIRST_SECTION(ntHeader);

    // Search for the section by name.
    for (WORD i = 0; i != ntHeader->FileHeader.NumberOfSections; ++i) {
        const auto& section = sectionHeaders[i];
        if (strncmp(reinterpret_cast<const char*>(section.Name),
                    sectname, sizeof(section.Name)) == 0) {
            return { base + section.VirtualAddress, section.Misc.VirtualSize };
        }
    }

    return { 0, 0 };
}

static
std::vector<Arch_ConstructorEntry>
GetConstructorEntries(HMODULE hModule, const char* sectname)
{
    std::vector<Arch_ConstructorEntry> result;

    // Find the section.
    const auto info = ImageNT(hModule).Find(sectname);

    // Done if not found or empty.
    const DWORD numEntries = info.size / sizeof(Arch_ConstructorEntry);
    if (numEntries == 0) {
        return result;
    }

    // Copy the entries for sorting.  We could sort in place but we want
    // to return a vector anyway.
    const Arch_ConstructorEntry* entries =
        reinterpret_cast<const Arch_ConstructorEntry*>(info.address);
    result.assign(entries, entries + numEntries);

    // Sort.
    std::sort(result.begin(), result.end(),
        [](const Arch_ConstructorEntry& lhs, const Arch_ConstructorEntry& rhs){
            return lhs.priority < rhs.priority;
        });

    return result;
}

static
void
RunConstructors(HMODULE hModule)
{
    // Leak this because we need it during application teardown.
    static std::set<HMODULE>* visited = new std::set<HMODULE>;

    // Do each HMODULE at most once.
    if (visited->insert(hModule).second) {
        // Execute in priority order.
        const auto entries = GetConstructorEntries(hModule, ".pxrctor");
        for (size_t i = 0, n = entries.size(); i != n; ++i) {
            if (entries[i].function && entries[i].version == 0u) {
                entries[i].function();
            }
        }
    }
}

static
void
RunDestructors(HMODULE hModule)
{
    // Leak this because we need it during application teardown.
    static std::set<HMODULE>* visited = new std::set<HMODULE>;

    // Do each HMODULE at most once.
    if (visited->insert(hModule).second) {
        // Execute in reverse priority order.
        const auto entries = GetConstructorEntries(hModule, ".pxrdtor");
        for (size_t i = entries.size(); i-- != 0; ) {
            if (entries[i].function && entries[i].version == 0u) {
                entries[i].function();
            }
        }
    }
}

// Returns the HMODULE for the module containing the address in ptr.
static
HMODULE
GetCurrentModule(void* ptr)
{
    HMODULE result;
    GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                      GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                      reinterpret_cast<LPCTSTR>(ptr),
                      &result);
    return result;
}

} // anonymous namespace

Arch_ConstructorInit::Arch_ConstructorInit()
{
    // Get the module containing this object, which we expect to be static
    // global.
    RunConstructors(GetCurrentModule(this));
}

Arch_ConstructorInit::~Arch_ConstructorInit()
{
    // Get the module containing this object, which we expect to be static
    // global.
    RunDestructors(GetCurrentModule(this));
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // ARCH_OS_WINDOWS
