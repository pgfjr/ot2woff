#pragma once

/*
    OT2Woff: An OpenType (PostScript/TrueType) to WOFF converter
    Copyright (c) 2020 by Peter Frane Jr.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.


    The author may be contacted via the e-mail address pfranejr@hotmail.com
*/

#include <stdexcept>
#include <string>
#include <stdio.h>
#include <vector>
#include <stdint.h>
#include <stdlib.h>
#include <zlib.h>

using namespace std;

#pragma comment(lib, "zdll.lib")

#define COPYRIGHT_NOTICE "OT2Woff v. 1.0\nCopyright (c) 2000 P.D. Frane Jr.\n"

#ifdef _MSC_VER
#define bswap16(x) _byteswap_ushort(x)
#define bswap32(x) _byteswap_ulong(x)
#else
#define bswap16(x) __builtin_bswap16(x)
#define bswap32(x) __builtin_bswap32(x)
#endif

const uint32_t OPENTYPE_TRUETYPE = 0x00010000;
const uint32_t OPENTYPE_TRUETYPE_MAC = 0x74727565;
const uint32_t OPENTYPE_CFF = 0x4F54544F;

typedef uint32_t offset32;
typedef unsigned char byte_t;

struct WOFF_header
{
    uint32_t m_signature{ 0 };	//0x774F4646 'wOFF'
    uint32_t m_flavor{ 0 };	//	The "sfnt version" of the input font.
    uint32_t m_length{ 0 };	//	Total size of the WOFF file.
    uint16_t m_num_tables{ 0 };	//	Number of entries in directory of font tables.
    uint16_t m_reserved{ 0 };	//	Reserved; set to zero.
    uint32_t m_total_sfnt_size{ 0 };	//	Total size needed for the uncompressed font data, including the sfnt header, directory, and font tables(including padding).
    uint16_t m_major_version{ 1 };	//	Major version of the WOFF file.
    uint16_t m_minor_version{ 0 };	//	Minor version of the WOFF file.
    uint32_t m_meta_offset{ 0 };	//	Offset to metadata block, from beginning of WOFF file.
    uint32_t m_meta_length{ 0 };	//	Length of compressed metadata block.
    uint32_t m_meta_orig_length{ 0 };	//	Uncompressed size of metadata block.
    uint32_t m_priv_offset{ 0 };	//	Offset to private data block, from beginning of WOFF file.
    uint32_t m_priv_length{ 0 };	//	Length of private data block.
};

struct table_directory_entry
{
    uint32_t	tag{ 0 };
    uint32_t	offset{ 0 };
    uint32_t	comp_length{ 0 };//	Length of the compressed data, excluding padding.
    uint32_t	orig_length{ 0 };//	Length of the uncompressed table, excluding padding.
    uint32_t	orig_checksum{ 0 };

};

struct offset_table
{
    uint32_t sfntVersion;
    uint16_t numTables;
    uint16_t searchRange;
    uint16_t entrySelector;
    uint16_t rangeShift;
};

struct table_record
{
    uint32_t table_tag{ 0 };
    uint32_t checksum{ 0 };
    offset32 offset{ 0 };
    uint32_t length{ 0 };
};

struct table_record_index
{
    uint16_t index{ 0 };
    offset32 offset{ 0 };
};

struct sorted_table_record
{
    uint16_t index{ 0 };
    offset32 offset{ 0 };
    uint32_t length{ 0 };
};

class OT2Woff
{
    FILE* m_input_file{ nullptr };
    FILE* m_output_file{ nullptr };
    string m_error;
    void clear()
    {
        if (m_input_file)
        {
            fclose(m_input_file);
        }
        if (m_output_file)
        {
            fclose(m_output_file);
        }

        m_input_file = m_output_file = nullptr;
    }
    void load_input_file(const char* otf_file)
    {
        m_input_file = fopen(otf_file, "rb");

        if (!m_input_file)
        {
            throw runtime_error("Unable to load the font");
        }
    }
    void create_output_file(const char* woff_file)
    {
        m_output_file = fopen(woff_file, "wb");

        if (!m_output_file)
        {
            throw runtime_error("Unable to create the WOFF file");
        }
    }
    void read_header(WOFF_header& hdr, uint16_t& num_tables)
    {
        offset_table offset_tbl = { 0 };

        if (fread(&offset_tbl, 1, sizeof(offset_tbl), m_input_file) != sizeof(offset_tbl))
        {
            throw runtime_error("Error reading the header of the input file");
        }
        else
        {
            uint32_t sfntVersion = bswap32(offset_tbl.sfntVersion);

            if (sfntVersion != OPENTYPE_CFF && sfntVersion != OPENTYPE_TRUETYPE &&
                sfntVersion != OPENTYPE_TRUETYPE_MAC)
            {
                throw runtime_error("Unknown file type");
            }

            //calculate_checksum_adjustment();

            hdr.m_signature = bswap32(0x774F4646); // wOFF
            hdr.m_flavor = offset_tbl.sfntVersion;
            hdr.m_num_tables = offset_tbl.numTables;

            num_tables = bswap16(offset_tbl.numTables);
        }
    }
    static int compare(const void* a, const void* b)
    {
        sorted_table_record* i = (sorted_table_record*)a;
        sorted_table_record* j = (sorted_table_record*)b;

        return i->offset - j->offset;
    }
    uint32_t read_table_record(uint16_t num_tables, table_directory_entry* tbl_directory_entry_data, sorted_table_record* sorted_table_data)
    {
        uint32_t total_sfnt_size;
        table_record tbl_record;

        total_sfnt_size = 12 + sizeof(tbl_record) * num_tables;

        for (uint16_t i = 0; i < num_tables; ++i)
        {
            if (fread(&tbl_record, 1, sizeof(tbl_record), m_input_file) != sizeof(tbl_record))
            {
                throw runtime_error("Error reading the font table records");
            }
            tbl_directory_entry_data[i].tag = tbl_record.table_tag;
            tbl_directory_entry_data[i].orig_checksum = tbl_record.checksum;
            tbl_directory_entry_data[i].orig_length = tbl_record.length;

            sorted_table_data[i].index = i;
            sorted_table_data[i].length = bswap32(tbl_record.length);
            sorted_table_data[i].offset = bswap32(tbl_record.offset);

            total_sfnt_size += (sorted_table_data[i].length + 3) & 0xFFFFFFFC;
        }

        qsort(sorted_table_data, num_tables, sizeof(*sorted_table_data), compare);

        return bswap32(total_sfnt_size);
    }
    void write_temp_header(uint16_t num_tables)
    {
        WOFF_header hdr;
        table_directory_entry entry;

        fwrite(&hdr, 1, sizeof(hdr), m_output_file);

        fwrite(&entry, num_tables, sizeof(entry), m_output_file);
    }
    uint32_t get_max_buffer_size(uint16_t num_tables, sorted_table_record* sorted_data)
    {
        uint32_t size = 0;

        for (uint16_t i = 0; i < num_tables; ++i)
        {
            size = sorted_data[i].length > size ? sorted_data[i].length : size;
        }

        return size;
    }
    void pad_table(byte_t* buffer, uint32_t length, uint32_t padded_length)
    {
        for (uint32_t i = length; i < padded_length; ++i)
        {
            buffer[i] = 0;
        }
    }
    int get_compression_level(const char* compression_level)
    {
        if (compression_level)
        {
            int num;

            if (sscanf(compression_level, "%d", &num) == 1)
            {
                if (num >= 0 && num <= 9)
                {
                    return num;
                }
                else
                {
                    printf("Error: Invalid compression level: %d. Value of 9 is used instead.", num);
                }
            }
            else
            {
                puts("Error: Value of the 4th parameter must be a number from 0 to 9 (highest compression). Value of 9 is used instead.");
            }
        }

        return 9;
    }
    uint32_t compress_and_write_table_data(uint16_t num_tables, table_directory_entry* tbl_directory_entry_data,
        sorted_table_record* sorted_table_data, const char* compression_level)
    {
        uint32_t buf_size = get_max_buffer_size(num_tables, sorted_table_data);
        uint32_t buf_size_padded = (buf_size + 3) & 0xFFFFFFFC;
        vector<Bytef> buf1(buf_size_padded), buf2(buf_size_padded);
        uint32_t table_length = 0;
        Bytef* buffer, * compressed_data;
        int level = 9;

        buffer = buf1.data();
        compressed_data = buf2.data();

        level = get_compression_level(compression_level);

        for (uint16_t i = 0; i < num_tables; ++i)
        {
            uint16_t index = sorted_table_data[i].index;
            uint32_t offset = sorted_table_data[i].offset;
            uLongf length = sorted_table_data[i].length;
            uint32_t padded_length = (length + 3) & 0xFFFFFFFC;
            table_directory_entry& entry = tbl_directory_entry_data[index];
            uLongf dest_len = buf_size;
            int ret;

            fseek(m_input_file, offset, SEEK_SET);

            fread(buffer, 1, (uint32_t)length, m_input_file);


            ret = compress2(compressed_data, &dest_len, buffer, (uLong)length, level);

            if (ret != Z_OK)
            {
                throw runtime_error("Error compressing the data. Zlib function'compress2()' failed");
            }
            else
            {
                entry.offset = bswap32(ftell(m_output_file));

                // if length of compressed data is smaller
                if (dest_len < length)
                {
                    uLongf new_dest_len = (dest_len + 3) & 0xFFFFFFFC; // pad to align on 4-byte boundary

                    entry.comp_length = bswap32((uint32_t)dest_len);

                    pad_table(compressed_data, (uint32_t)dest_len, (uint32_t)new_dest_len);

                    fwrite(compressed_data, 1, new_dest_len, m_output_file);

                    table_length += new_dest_len;
                }
                else
                {
                    uLongf new_length = (length + 3) & 0xFFFFFFFC;

                    entry.comp_length = entry.orig_length;

                    pad_table(buffer, (uint32_t)length, (uint32_t)new_length);

                    fwrite(buffer, 1, (size_t)new_length, m_output_file);

                    table_length += (uint32_t)new_length;
                }
            }
        }

        return table_length;
    }
    void rewrite_header(WOFF_header& hdr)
    {
        fseek(m_output_file, 0, SEEK_SET);

        fwrite(&hdr, 1, sizeof(hdr), m_output_file);
    }
    void rewrite_table_directory_entries(uint16_t num_tables, table_directory_entry* tbl_directory_entry_data)
    {
        fwrite(tbl_directory_entry_data, 1, num_tables * sizeof(*tbl_directory_entry_data), m_output_file);
    }
    void parse_input_file(const char* compression_level)
    {
        WOFF_header hdr;
        uint16_t num_tables;
        vector<sorted_table_record> sorted_tbl_record;
        vector<table_directory_entry> tbl_directory_entry;
        uint32_t table_length;
        table_directory_entry* tbl_directory_entry_data;
        sorted_table_record* sorted_tbl_record_data;

        read_header(hdr, num_tables);

        sorted_tbl_record.resize(num_tables);

        tbl_directory_entry.resize(num_tables);

        tbl_directory_entry_data = tbl_directory_entry.data();
        sorted_tbl_record_data = sorted_tbl_record.data();

        hdr.m_total_sfnt_size = read_table_record(num_tables, tbl_directory_entry_data, sorted_tbl_record_data);

        write_temp_header(num_tables);

        table_length = compress_and_write_table_data(num_tables, tbl_directory_entry_data, sorted_tbl_record_data, compression_level);

        // total size of the woff file: header + no. of table_directory_entry + table_length

        table_length += sizeof(hdr) + (num_tables * sizeof(*tbl_directory_entry_data));

        hdr.m_length = bswap32(table_length);

        rewrite_header(hdr);

        rewrite_table_directory_entries(num_tables, tbl_directory_entry_data);
    }
public:
    OT2Woff() : m_error()
    {}
    ~OT2Woff()
    {}
    string error() const
    {
        return m_error;
    }
    bool convert(const char* otf_file, const char* woff_file, const char *compression_level)
    {
        bool result = true;

        try
        {
            puts(COPYRIGHT_NOTICE);
            load_input_file(otf_file);
            create_output_file(woff_file);
            parse_input_file(compression_level);
        }
        catch (const exception& ex)
        {
            m_error = ex.what();

            result = false;
        }

        clear();

        return result;
    }
};
