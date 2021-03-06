#pragma once

#include <span>
#include <optional>

#include <LibElf/Generator.hpp>
#include <LibElf/SymbolTable.hpp>
#include <LibElf/RelocationTable.hpp>

#include <Kernel/Interface/Types.hpp>

class FileSystem {
public:
    explicit FileSystem(Elf::Generator& generator);
    ~FileSystem();

    uint32_t add_file(
        Elf::MemoryStream&,
        Kernel::ModeFlags mode = Kernel::ModeFlags::Regular | Kernel::ModeFlags::DefaultPermissions,
        uint32_t inode_number = 0,
        size_t *load_offset = nullptr);

    uint32_t add_host_file(
        std::string_view path,
        Kernel::ModeFlags mode = Kernel::ModeFlags::Regular | Kernel::ModeFlags::DefaultPermissions);

    uint32_t add_directory(std::map<std::string, uint32_t>& files, uint32_t inode_number = 0);
    uint32_t add_root_directory(std::map<std::string, uint32_t>& files);

    void finalize();

private:
    Elf::Generator& m_generator;
    bool m_finalized = false;

    std::optional<Elf::RelocationTable> m_data_relocs;
    std::optional<size_t> m_data_index;
    std::optional<size_t> m_base_symbol;

    Elf::MemoryStream m_data_stream;

    std::map<uint32_t, uint32_t> m_inode_to_offset;

    size_t m_next_inode = 3;
};
