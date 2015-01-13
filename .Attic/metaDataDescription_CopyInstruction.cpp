class MetaDataDescription
{
	/// \brief Copy instruction to move data from a record to another for an alter table command
	struct CopyInstruction
	{
		std::size_t dst_ofs;		///< destination offset in the new record
		std::size_t src_ofs;		///< source offset in the old record
		std::size_t bytesize;		///< number of bytes to copy
	
		/// \brief Copy constructor
		CopyInstruction( const CopyInstruction& o)
			:dst_ofs(o.dst_ofs),src_ofs(o.src_ofs),bytesize(o.bytesize){}
		/// \brief Constructor
		CopyInstruction( std::size_t dst_ofs_, std::size_t src_ofs_, std::size_t bytesize_)
			:dst_ofs(dst_ofs_),src_ofs(src_ofs_),bytesize(bytesize_){}
	};
	
	/// \brief Get the list of copy (move data) instructions for the alter table command from 'o' to 'this'
	/// \param[in] o old table description
	std::vector<CopyInstruction> getAlterTableInstructions( const MetaDataDescription& o);
	
	/// \brief Alter the table by renaming one element
	/// \param[in] newname new name of the element
	/// \param[in] oldname old name of the element
	void renameElement( const std::string& newname, const std::string& oldname);
};


bool CompareCopyInstructionLess( const CopyInstruction& aa, const CopyInstruction& bb)
{
	if (dst_ofs < o.dst_ofs) return true;
	if (dst_ofs > o.dst_ofs) return false;
	return (src_ofs < o.src_ofs);
}

std::vector<CopyInstruction> MetaDataDescription::getAlterTableInstructions( const MetaDataDescription& o)
{
	// [1] Generate alter table move instructions:
	std::vector<CopyInstruction> instr;

	std::map<std::string,std::size_t>::const_iterator
		ni = o.m_namemap.begin(), ne = o.m_namemap.end();
	for (; ni != ne; ++ni)
	{
		std::map<std::string,std::size_t>::const_iterator
			fi = m_namemap.find( ni->first);
		if (fi != m_namemap.end())
		{
			MetaDataElement src_elem = o.m_ar[ ni->second];
			MetaDataElement dst_elem = m_ar[ fi->second];

			if (src_elem.bytesize != dst_elem.bytesize)
			{
				throw std::runtime_error("cannot translate meta data block (types with different sizes)");
			}
			CopyInstruction cpi( dst_elem.ofs(), src_elem.ofs(), dst_elem.size());
			instr.push_back( cpi);
		}
	}

	// [2] Join neighbours in generated instruction list:
	std::vector<CopyInstruction> rt;
	std::sort( instr.begin(), instr.end(), CompareCopyInstructionLess);
	std::vector<CopyInstruction>::const_iterator ci = instr.begin(), ce = instr.end();
	while (ci != ce)
	{
		CopyInstruction cpi = *ci;
		for (++ci; ci != ce; ++ci)
		{
			if (ci->src_ofs + ci->bytesize)
		}
	}
}
