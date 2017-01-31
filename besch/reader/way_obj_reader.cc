#include <stdio.h>
#include "../../simdebug.h"
#include "../../utils/simstring.h"

#include "../way_obj_besch.h"
#include "../../obj/wayobj.h"

#include "way_obj_reader.h"
#include "../obj_node_info.h"
#include "../../network/pakset_info.h"


void way_obj_reader_t::register_obj(obj_besch_t *&data)
{
    way_obj_besch_t *besch = static_cast<way_obj_besch_t *>(data);
    wayobj_t::register_besch(besch);

	checksum_t *chk = new checksum_t();
	besch->calc_checksum(chk);
	pakset_info_t::append(besch->get_name(), chk);
}


bool way_obj_reader_t::successfully_loaded() const
{
    return wayobj_t::alles_geladen();
}


obj_besch_t * way_obj_reader_t::read_node(FILE *fp, obj_node_info_t &node)
{
	ALLOCA(char, besch_buf, node.size);

	way_obj_besch_t *besch = new way_obj_besch_t();
	// DBG_DEBUG("way_reader_t::read_node()", "node size = %d", node.size);

	// Hajo: Read data
	fread(besch_buf, node.size, 1, fp);
	char * p = besch_buf;

	// Hajo: old versions of PAK files have no version stamp.
	// But we know, the higher most bit was always cleared.
	const uint16 v = decode_uint16(p);
	uint16 version = v & 0x7FFF;

	// Whether the read file is from Simutrans-Experimental
	//@author: jamespetts

	way_constraints_of_way_t way_constraints;
	const bool experimental = version > 0 ? v & EXP_VER : false;
	uint16 experimental_version = 0;
	if(experimental)
	{
		// Experimental version to start at 0 and increment.
		version = version & EXP_VER ? version & 0x3FFF : 0;
		while(version > 0x100)
		{
			version -= 0x100;
			experimental_version ++;
		}
		experimental_version -=1;
	}

	if(version==1) {
		// Versioned node, version 3
		besch->cost = decode_uint32(p);
		besch->maintenance = decode_uint32(p);
		besch->topspeed = decode_uint32(p);
		besch->intro_date = decode_uint16(p);
		besch->obsolete_date = decode_uint16(p);
		besch->wt = decode_uint8(p);
		besch->own_wtyp = decode_uint8(p);
		if(experimental)
		{
			if(experimental_version == 0)
			{
				way_constraints.set_permissive(decode_uint8(p));
				way_constraints.set_prohibitive(decode_uint8(p));
			}
			else
			{
				dbg->fatal( "way_obj_reader_t::read_node()","Incompatible pak file version for Simutrans-E, number %i", experimental_version );
			}
		}
	}
	else {
		dbg->fatal("way_obj_reader_t::read_node()","Invalid version %d", version);
	}

	besch->set_way_constraints(way_constraints);

	besch->base_cost = besch->cost;
	besch->base_maintenance = besch->maintenance;
	// Way objects do not have a separate top speed for gradients,
	// but still need this value to be set to avoid it being zero.
	besch->topspeed_gradient_1 = besch->topspeed_gradient_2 = besch->topspeed;

  DBG_DEBUG("way_obj_reader_t::read_node()",
	     "version=%d cost=%d maintenance=%d topspeed=%d wtype=%d styp=%d intro_year=%i",
	     version,
	     besch->cost,
	     besch->maintenance,
	     besch->topspeed,
	     besch->wt,
	     besch->own_wtyp,
	     besch->intro_date/12);

  return besch;
}
