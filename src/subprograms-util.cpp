#include "subprograms-util.hpp"

using std::cerr;
using std::map;
using std::make_shared;
using boost::optional;
using std::set;
using namespace dwarf;
using dwarf::core::iterator_base;
using dwarf::core::iterator_df;
using dwarf::core::iterator_sibs;
using dwarf::core::type_die;
using dwarf::core::subprogram_die;
using dwarf::core::compile_unit_die;

using dwarf::lib::Dwarf_Off;
using dwarf::lib::Dwarf_Addr;

namespace allocs {
namespace tool {

void
gather_defined_subprograms(root_die& root,
	subprogram_vaddr_interval_map_t& out_by_vaddr,
	map<subprogram_key, iterator_df<subprogram_die> >& out_by_key
	)
{
	/* FIXME: could speed this up by cutting off the search underneath
	 * certain tags. But which? Subprograms need not be grandchildren of the root,
	 * if we have namespaces or the like. */
	for (iterator_df<> i = root.begin(); i != root.end(); ++i)
	{
		if (i.is_a<subprogram_die>())
		{
			auto i_cu = i.enclosing_cu();
			
			iterator_df<subprogram_die> i_subp = i;
			// only add real, defined subprograms to the list
			if (
				// not a declaration
				(!i_subp->get_declaration() || !*i_subp->get_declaration()) &&
				// not an "abstract instance"
				// FIXME: lift this up into libdwarfpp
				(!i_subp->get_inline() || *i_subp->get_inline() == DW_INL_not_inlined)
			)
			{
				string sourcefile_name = i_subp->get_decl_file() ? 
					i_cu->source_file_name(*i_subp->get_decl_file())
					: "(unknown source file)";
				string comp_dir = i_cu->get_comp_dir() ? *i_cu->get_comp_dir() : "";

				string subp_name;
				if (i_subp.name_here()) subp_name = *i_subp.name_here();
				else 
				{
					std::ostringstream s;
					s << "0x" << std::hex << i_subp.offset_here();
					subp_name = s.str();
				}

				subprogram_key k(subp_name, sourcefile_name, comp_dir);
				auto ret = out_by_key.insert(make_pair(k, i_subp));
				if (!ret.second)
				{
					/* This means that "the same value already existed". */
					cerr << "Warning: subprogram " << *i_subp
						<< " already in subprograms_list as " 
						<< ret.first->first.subprogram_name() 
						<< " (in " 
						<< ret.first->first.sourcefile_name()
						<< ", compiled in " << ret.first->first.comp_dir()
						<< ")"
						<< endl;
				}
				auto all_intervals = i_subp->file_relative_intervals(root, nullptr, nullptr);
				cerr << "Adding subprogram " << i_subp.summary()
					<< " with intervals: ";
				for (auto i_int = all_intervals.begin(); i_int != all_intervals.end();
					++i_int)
				{
					set< pair< subprogram_key, iterator_df<subprogram_die> > > singleton_set;
					singleton_set.insert(make_pair(k, i_subp));
					if (i_int != all_intervals.begin()) cerr << ", ";
					cerr << std::hex << i_int->first << std::dec;
					out_by_vaddr.insert(make_pair(
						i_int->first,
						singleton_set
					));
				}
				cerr << endl;
			}
		}
	}
}

}
}
