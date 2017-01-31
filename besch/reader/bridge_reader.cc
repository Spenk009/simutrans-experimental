#include <stdio.h>
#include "../../simdebug.h"

#include "../../bauer/brueckenbauer.h"
#include "../bruecke_besch.h"
#include "../intro_dates.h"

#include "bridge_reader.h"
#include "../obj_node_info.h"
#include "../../network/pakset_info.h"


void bridge_reader_t::register_obj(obj_besch_t *&data)
{
	bruecke_besch_t *besch = static_cast<bruecke_besch_t *>(data);
	brueckenbauer_t::register_besch(besch);

	checksum_t *chk = new checksum_t();
	besch->calc_checksum(chk);
	pakset_info_t::append(besch->get_name(), chk);
}


obj_besch_t * bridge_reader_t::read_node(FILE *fp, obj_node_info_t &node)
{
	// DBG_DEBUG("bridge_reader_t::read_node()", "called");
	ALLOCA(char, besch_buf, node.size);

	bruecke_besch_t *besch = new bruecke_besch_t();

	// Hajo: Read data
	fread(besch_buf, node.size, 1, fp);

	char * p = besch_buf;

	// Hajo: old versions of PAK files have no version stamp.
	// But we know, the higher most bit was always cleared.

	const uint16 v = decode_uint16(p);
	int version = v & 0x8000 ? v & 0x7FFF : 0;

	// Whether the read file is from Simutrans-Experimental
	//@author: jamespetts

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

	// some defaults
	besch->maintenance = 800;
	besch->pillars_every = 0;
	besch->pillars_asymmetric = false;
	besch->max_length = 0;
	besch->max_height = 0;
	besch->max_weight = 999;
	besch->intro_date = DEFAULT_INTRO_DATE*12;
	besch->obsolete_date = DEFAULT_RETIRE_DATE*12;
	besch->number_seasons = 0;
	way_constraints_of_way_t way_constraints;

	if(version == 1) {
		// Versioned node, version 1

		besch->wt = (uint8)decode_uint16(p);
		besch->topspeed = decode_uint16(p);
		besch->cost = decode_uint32(p);

	} else if (version == 2) {

		// Versioned node, version 2

		besch->topspeed = decode_uint16(p);
		besch->cost = decode_uint32(p);
		besch->maintenance = decode_uint32(p);
		besch->wt = decode_uint8(p);

	} else if (version == 3) {

		// Versioned node, version 3
		// pillars added

		besch->topspeed = decode_uint16(p);
		besch->cost = decode_uint32(p);
		besch->maintenance = decode_uint32(p);
		besch->wt = decode_uint8(p);
		besch->pillars_every = decode_uint8(p);
		besch->max_length = 0;

	} else if (version == 4) {

		// Versioned node, version 3
		// pillars added

		besch->topspeed = decode_uint16(p);
		besch->cost = decode_uint32(p);
		besch->maintenance = decode_uint32(p);
		besch->wt = decode_uint8(p);
		besch->pillars_every = decode_uint8(p);
		besch->max_length = decode_uint8(p);

	} else if (version == 5) {

		// Versioned node, version 5
		// timeline

		besch->topspeed = decode_uint16(p);
		besch->cost = decode_uint32(p);
		besch->maintenance = decode_uint32(p);
		besch->wt = decode_uint8(p);
		besch->pillars_every = decode_uint8(p);
		besch->max_length = decode_uint8(p);
		besch->intro_date = decode_uint16(p);
		besch->obsolete_date = decode_uint16(p);

	} else if (version == 6) {

		// Versioned node, version 6
		// snow

		besch->topspeed = decode_uint16(p);
		besch->cost = decode_uint32(p);
		besch->maintenance = decode_uint32(p);
		besch->wt = decode_uint8(p);
		besch->pillars_every = decode_uint8(p);
		besch->max_length = decode_uint8(p);
		besch->intro_date = decode_uint16(p);
		besch->obsolete_date = decode_uint16(p);
		besch->number_seasons = decode_uint8(p);

	}
	else if (version==7  ||  version==8) {

		// Versioned node, version 7/8
		// max_height, assymetric pillars

		besch->topspeed = decode_uint16(p);
		besch->cost = decode_uint32(p);
		besch->maintenance = decode_uint32(p);
		besch->wt = decode_uint8(p);
		besch->pillars_every = decode_uint8(p);
		besch->max_length = decode_uint8(p);
		besch->intro_date = decode_uint16(p);
		besch->obsolete_date = decode_uint16(p);
		besch->pillars_asymmetric = (decode_uint8(p)!=0);
		besch->max_height = decode_uint8(p);
		besch->number_seasons = decode_uint8(p);
		if(experimental)
		{
			besch->max_weight = besch->axle_load = decode_uint32(p);
			way_constraints.set_permissive(decode_uint8(p));
			way_constraints.set_prohibitive(decode_uint8(p));
			if(experimental_version == 1)
			{
				besch->topspeed_gradient_1 = decode_uint16(p);
				besch->topspeed_gradient_2 = decode_uint16(p);
				besch->max_altitude = decode_sint8(p);
				besch->max_vehicles_on_tile = decode_uint8(p);
				besch->has_own_way_graphics = decode_uint8(p);
				besch->has_way = decode_uint8(p);
			}
			if(experimental_version > 1)
			{
				dbg->fatal("bridge_reader_t::read_node()","Incompatible pak file version for Simutrans-Ex, number %i", experimental_version);
			}
		}

	}
	else if (version==9) {

		besch->topspeed = decode_uint16(p);
		besch->cost = decode_uint32(p);
		besch->maintenance = decode_uint32(p);
		besch->wt = decode_uint8(p);
		besch->pillars_every = decode_uint8(p);
		besch->max_length = decode_uint8(p);
		besch->intro_date = decode_uint16(p);
		besch->obsolete_date = decode_uint16(p);
		besch->pillars_asymmetric = (decode_uint8(p)!=0);
		besch->max_height = decode_uint8(p);
		besch->axle_load = decode_uint16(p);	// new
		if(experimental)
		{
			besch->max_weight = decode_uint32(p); // DIFFERENT to axle load.
			way_constraints.set_permissive(decode_uint8(p));
			way_constraints.set_prohibitive(decode_uint8(p));
			if(experimental_version == 1)
			{
				besch->topspeed_gradient_1 = decode_uint16(p);
				besch->topspeed_gradient_2 = decode_uint16(p);
				besch->max_altitude = decode_sint8(p);
				besch->max_vehicles_on_tile = decode_uint8(p);
				besch->has_own_way_graphics = decode_uint8(p);
				besch->has_way = decode_uint8(p);
			}
			if(experimental_version > 1)
			{
				dbg->fatal("bridge_reader_t::read_node()","Incompatible pak file version for Simutrans-Ex, number %i", experimental_version);
			}
		}
		besch->number_seasons = decode_uint8(p);

	}
	else {
		// old node, version 0

		besch->wt = (uint8)v;
		decode_uint16(p);                    // Menupos, no more used
		besch->cost = decode_uint32(p);
		besch->topspeed = 999;               // Safe default ...
	}

	besch->set_way_constraints(way_constraints);

	if(experimental_version < 1 || !experimental)
	{
		besch->topspeed_gradient_1 = besch->topspeed_gradient_2 = besch->topspeed;
		besch->max_altitude = 0;
		besch->max_vehicles_on_tile = 251;
		besch->has_own_way_graphics = true;
		besch->has_way = false;
	}

	// pillars cannot be heigher than this to avoid drawing errors
	if(besch->pillars_every>0  &&  besch->max_height==0) {
		besch->max_height = 7;
	}
	// indicate for different copyright/name lookup
	besch->offset = version<8 ? 0 : 2;

	besch->base_cost = besch->cost;
	besch->base_maintenance = besch->maintenance;

	if(  version < 9  ) {
		besch->axle_load = 9999;
	}

	DBG_DEBUG("bridge_reader_t::read_node()",
		"version=%d, waytype=%d, price=%d, topspeed=%d, pillars=%i, max_length=%i, max_weight%d, axle_load=%i",
		version, besch->wt, besch->cost, besch->topspeed,besch->pillars_every,besch->max_length,besch->max_weight,besch->axle_load);

  return besch;
}
