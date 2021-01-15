#pragma once

#include "util/types.hpp"
#include "util/endian.hpp"
#include "../../Utilities/File.h"

namespace PFD_positions
{ 
	enum
	{
		header = 0,
		table_header = 0x60,
		x_table = 0x78,
		pfd = 0x240,
	};
}

struct PFDHeader
{
	u64 magic;
	be_t<u64> version; // 3
	u8 header_table_iv[16];
	u8 ytable_hmac[20];
	u8 xtable_hmax[20];
	u8 file_hmax[20];
	u8 padding[4];
};

struct PFDTableHeader
{
	be_t<u64> xytable_entry_count;
	be_t<u64> pfd_reserved_entries;
	be_t<u64> pfd_used_entries;
};

struct PFDEntry
{
    be_t<u64> vidx;
    char filename[65];
    char padding[7];
    u8 file_key[64];
    u8 hmacs[4][20];
    char padding2[40];
    be_t<u64> file_size;
};

struct PFDData
{
	PFDHeader header;
	PFDTableHeader table;
	std::vector<be_t<u64>> xtable;
    std::vector<std::array<u8, 16>> ytable;
    std::vector<PFDEntry> pfd_table;
};

namespace pfd
{
	PFDHeader load_header(const fs::file& f);
    PFDTableHeader load_table_header(const fs::file& f);
    PFDData load(const fs::file& f);
	std::vector<u8> decrypt(PFDData pfd_data, std::string_view filename, std::vector<u8> in, be_t<u128> key);
    std::vector<u8> encrypt(PFDData pfd_data, std::string_view filename, std::vector<u8> in, be_t<u128> key);
}