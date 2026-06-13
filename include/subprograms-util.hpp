#ifndef ALLOCSTOOL_SUBPROGRAM_UTIL_HPP_
#define ALLOCSTOOL_SUBPROGRAM_UTIL_HPP_

#include <fstream>
#include <sstream>
#include <map>
#include <set>
#include <string>
#include <memory>
#include <boost/icl/interval_map.hpp>
#include <dwarfpp/lib.hpp>

#include "stickyroot.hpp"

namespace allocs {
namespace tool {

using std::map;
using std::pair;
using namespace dwarf;
using dwarf::lib::Dwarf_Off;
using dwarf::core::iterator_df;
using dwarf::core::type_die;
using dwarf::core::subprogram_die;

struct subprogram_key : public pair< pair<string, string>, string > // ordering for free
{
	subprogram_key(const string& subprogram_name, const string& sourcefile_name, 
		const string& comp_dir) : pair(make_pair(subprogram_name, sourcefile_name), comp_dir) {}
	string subprogram_name() const { return first.first; }
	string sourcefile_name() const { return first.second; }
	string comp_dir() const { return second; }
};

/* We gather subprograms by the ranges they cover
 * AND by their identity (key). */
typedef boost::icl::interval_map<
	Dwarf_Off,
	/* It's a set only so that we can detect and warn about overlaps... */
	std::set< pair< subprogram_key, iterator_df<subprogram_die> > >
> subprogram_vaddr_interval_map_t;

void
gather_defined_subprograms(root_die& root,
	subprogram_vaddr_interval_map_t& out_by_vaddr,
	map<subprogram_key, iterator_df<subprogram_die> >& out_by_key
	);

/* This is useful for iterating over the contents of a subprogram,
 * without getting deceived by stuff that might be nested within a local
 * type, e.g. the member functions of that local type (which contain
 * stuff that does not belong to this subprogram in the same way). */
struct iterator_bf_skipping_types : public core::iterator_bf<>
{
	typedef core::iterator_bf<> super;
	void increment(unsigned min_depth)
	{
		/* The idea here is not that we skip types per se.
		 * It's that we skip the children of types, e.g.
		 * local vars or formals that actually belong to
		 * methods. Remember that subprograms are types.
		 * Also remember that we're allowed to start above
		 * the minimum depth. */
		if (*this != END && depth() < min_depth)
		{
			this->increment_skipping_siblings();
		}
		else if (tag_here() != DW_TAG_subprogram &&
			spec_here().tag_is_type(tag_here()))
		{
			this->increment_skipping_subtree();
		} else this->super::increment();
		if (*this != END && depth() < min_depth) *this = END;
	}
	void increment() { this->increment(0); }
	// forward constructors
	using core::iterator_bf<>::iterator_bf;
};

}} /* end namespaces */

#endif
