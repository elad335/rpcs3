//#include "stdafx.h"

#include "PFD.h"
#include "Crypto/utils.h"

constexpr std::array<u8, 16> Header_key = 
{
    0xD4, 0x13, 0xB8, 0x96, 0x63, 0xE1, 0xFE, 0x9F, 0x75, 0x14, 0x3D, 0x3B, 0xB4, 0x56, 0x52, 0x74
};

namespace pfd
{
	PFDHeader load_header(const fs::file& f)
	{
		PFDHeader header{}, out{};

		if (!f || f.size() < sizeof(PFDHeader)) return out;

		f.seek(PFD_positions::header);
		f.read(header);

        if (header.magic != "\0\0\0\0PFDB"_u64 || header.version != 3u)
        {
            return out;
        }

		const usz dec_size = sizeof(PFDHeader) - ::offset32(&PFDHeader::ytable_hmac);

        std::memcpy(&out, &header, sizeof(PFDHeader) - dec_size);

		aescbc128_decrypt(const_cast<u8*>(Header_key.data()), header.header_table_iv, header.ytable_hmac, out.ytable_hmac, dec_size);

        return out;
	}

    PFDTableHeader load_table_header(const fs::file& f)
    {
        PFDTableHeader out{};

        if (!f || f.size() < PFD_positions::table_header + sizeof(PFDTableHeader)) return out;

        f.seek(PFD_positions::table_header);
        f.read(out);

        return out;
    }

    PFDData load(const fs::file& f)
    {
        PFDData out{};

        if (!f || f.size() != 0x8000) return out;

        out.header = load_header(f);
        out.table = load_table_header(f);

        f.seek(PFD_positions::x_table);

        out.xtable.resize(out.table.xytable_entry_count);
        f.read(out.xtable);

        out.pfd_table.resize(out.table.pfd_reserved_entries);
        f.read(out.pfd_table);

        return out;
    }

	std::vector<u8> decrypt(PFDData pfd_data, std::string_view filename, std::vector<u8> in, be_t<u128> key)
    {
        std::vector<u8> out;
        out.resize(in.size());

        for (auto& entry : pfd_data.pfd_table)
        {
            if (entry.filename == filename)
            {
                aescbc128_decrypt(const_cast<u8*>(Header_key.data()), entry, in.data(), out.data(), in.size());
            }
        }
    }

    std::vector<u8> encrypt(PFDData pfd_data, std::string_view filename, std::vector<u8> in, be_t<u128> key)
    {
        // TODO
        return {};
    }
}
