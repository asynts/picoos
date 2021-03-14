#pragma once

#include <cassert>
#include <vector>
#include <cstring>

#include <elf.h>

#include "BufferStream.hpp"

// FIXME: remove
#include <iostream>

class ElfGenerator {
public:
    ElfGenerator()
    {
        // We write the elf header in ElfGenerator::finalize
        m_stream.seek(sizeof(Elf32_Ehdr));

        create_section("", 0, 0, 0, SHT_NULL);
    }

    size_t append_section(
        std::string_view name,
        BufferStream& stream,
        Elf32_Word type = SHT_PROGBITS,
        Elf32_Word flags = SHF_ALLOC)
    {
        std::cout << "ElfGenerator::append_section name='" << name << "' size=" << stream.size() << '\n';

        Elf32_Addr address = m_stream.offset() - sizeof(Elf32_Ehdr);
        Elf32_Off offset = m_stream.offset();
        Elf32_Word size = stream.size();

        m_stream.write_bytes(stream);

        return create_section(name, address, offset, size, type, flags);
    }

    size_t create_section(
        std::string_view name,
        Elf32_Addr address,
        Elf32_Off offset,
        Elf32_Word size,
        Elf32_Word type = SHT_PROGBITS,
        Elf32_Word flags = SHF_ALLOC)
    {
        std::cout << "ElfGenerator::create_section name='" << name << "' address=" << address << " offset=" << offset << " size=" << size << '\n';

        Elf32_Shdr shdr;
        shdr.sh_addr = 0;
        shdr.sh_addralign = 4;
        shdr.sh_entsize = 0;
        shdr.sh_flags = flags;
        shdr.sh_info = 0;
        shdr.sh_link = 0;
        shdr.sh_name = append_section_name(name);
        shdr.sh_offset = m_stream.offset();
        shdr.sh_size = 0;
        shdr.sh_type = type;
        
        size_t index = m_sections.size();
        m_sections.push_back(shdr);
        return index;
    }

    BufferStream finalize() &&
    {
        assert(!m_finalized);
        m_finalized = true;

        size_t section_offset;
        size_t shstrtab_section_index;
        encode_sections(section_offset, shstrtab_section_index);

        encode_header(section_offset, shstrtab_section_index);

        m_stream.seek(0);
        return std::move(m_stream);
    }

private:
    size_t append_section_name(std::string_view name)
    {
        size_t offset = m_shstrtab_stream.offset();
        m_shstrtab_stream.write_bytes({ (const uint8_t*)name.data() , name.size() });
        m_shstrtab_stream.write_object((uint8_t)0);
        return offset;
    }

    void encode_sections(size_t& section_offset, size_t& shstrtab_section_index)
    {
        shstrtab_section_index = append_section(".shstrtab", m_shstrtab_stream, SHT_STRTAB, 0);

        printf("Putting sections at %zu\n", m_stream.offset());
        section_offset = m_stream.offset();

        for (const Elf32_Shdr& section : m_sections)
            m_stream.write_object(section);
    }

    void encode_header(size_t section_offset, size_t shstrtab_section_index)
    {
        Elf32_Ehdr ehdr;
        ehdr.e_ehsize = sizeof(Elf32_Ehdr);
        ehdr.e_entry = 0;
        ehdr.e_flags = 0x05000000;
        
        memcpy(ehdr.e_ident, ELFMAG, SELFMAG);
        ehdr.e_ident[EI_CLASS] = ELFCLASS32;
        ehdr.e_ident[EI_DATA] = ELFDATA2LSB;
        ehdr.e_ident[EI_VERSION] = EV_CURRENT;
        ehdr.e_ident[EI_OSABI] = ELFOSABI_NONE;
        ehdr.e_ident[EI_ABIVERSION] = 0;

        ehdr.e_machine = EM_ARM;
        ehdr.e_phentsize = sizeof(Elf32_Phdr);
        ehdr.e_phnum = 0;
        ehdr.e_phoff = 0;
        ehdr.e_shentsize = sizeof(Elf32_Shdr);
        ehdr.e_shnum = m_sections.size();
        ehdr.e_shoff = section_offset;
        ehdr.e_shstrndx = shstrtab_section_index;
        ehdr.e_type = ET_REL;
        ehdr.e_version = EV_CURRENT;

        m_stream.seek(0);
        m_stream.write_object(ehdr);
    }

    BufferStream m_stream;

    bool m_finalized = false;
    std::vector<Elf32_Shdr> m_sections;
    BufferStream m_shstrtab_stream;
};
