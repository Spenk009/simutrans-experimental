/*
 * Roads for Simutrans
 *
 * Revised January 2001
 * Hj. Malthaner
 */

#include <stdio.h>

#include "strasse.h"
#include "../../simworld.h"
#include "../../obj/bruecke.h"
#include "../../obj/tunnel.h"
#include "../../dataobj/loadsave.h"
#include "../../descriptor/way_desc.h"
#include "../../descriptor/tunnel_desc.h"
#include "../../bauer/wegbauer.h"
#include "../../dataobj/translator.h"
#include "../../dataobj/ribi.h"
#include "../../utils/cbuffer_t.h"
#include "../../vehicle/simvehicle.h" /* for calc_direction */
#include "../../obj/wayobj.h"

const way_desc_t *strasse_t::default_strasse=NULL;


void strasse_t::set_gehweg(bool janein)
{
	grund_t *gr = welt->lookup(get_pos());
	wayobj_t *wo = gr ? gr->get_wayobj(road_wt) : NULL;

	if (wo && wo->get_desc()->is_noise_barrier()) {
		janein = false;
	}

	weg_t::set_gehweg(janein);
	if(janein && get_desc()) 
	{
		if(welt->get_settings().get_town_road_speed_limit())
		{
			set_max_speed(min(welt->get_settings().get_town_road_speed_limit(), get_desc()->get_topspeed()));
		}
		else
		{
			set_max_speed(get_desc()->get_topspeed());
		}
	}
	if(!janein && get_desc()) {
		set_max_speed(get_desc()->get_topspeed());
	}
	if(gr) {
		gr->calc_image();
	}
}



strasse_t::strasse_t(loadsave_t *file) : weg_t(road_wt)
{
	rdwr(file);
}


strasse_t::strasse_t() : weg_t(road_wt)
{
	set_gehweg(false);
	set_desc(default_strasse);
}



void strasse_t::rdwr(loadsave_t *file)
{
	xml_tag_t s( file, "strasse_t" );

	weg_t::rdwr(file);

	if(file->get_version()<89000) {
		bool gehweg;
		file->rdwr_bool(gehweg);
		set_gehweg(gehweg);
	}

	if(file->is_saving()) 
	{
		const char *s = get_desc()->get_name();
		file->rdwr_str(s);
		if(file->get_extended_version() >= 12)
		{
			s = replacement_way ? replacement_way->get_name() : ""; 
			file->rdwr_str(s);
		}
	}
	else
	{
		char bname[128];
		file->rdwr_str(bname, lengthof(bname));
		const way_desc_t *desc = way_builder_t::get_desc(bname);

#ifndef SPECIAL_RESCUE_12_3
		const way_desc_t* loaded_replacement_way = NULL;
		if(file->get_extended_version() >= 12)
		{
			char rbname[128];
			file->rdwr_str(rbname, lengthof(rbname));
			loaded_replacement_way = way_builder_t::get_desc(rbname);
		}
#endif

		const sint32 old_max_speed = get_max_speed();
		const uint32 old_max_axle_load = get_max_axle_load();
		const uint32 old_bridge_weight_limit = get_bridge_weight_limit();
		if(desc==NULL) {
			desc = way_builder_t::get_desc(translator::compatibility_name(bname));
			if(desc==NULL) {
				desc = default_strasse;
				welt->add_missing_paks( bname, karte_t::MISSING_WAY );
			}
			dbg->warning("strasse_t::rdwr()", "Unknown street %s replaced by %s (old_max_speed %i)", bname, desc->get_name(), old_max_speed );
		}

		set_desc(desc, file->get_extended_version() >= 12);
#ifndef SPECIAL_RESCUE_12_3
		if(file->get_extended_version() >= 12)
		{
			replacement_way = loaded_replacement_way;
		}
#endif

		if(old_max_speed>0) {
			set_max_speed(old_max_speed);
		}
		if(old_max_axle_load > 0)
		{
			set_max_axle_load(old_max_axle_load);
		}
		if(old_bridge_weight_limit > 0)
		{
			set_bridge_weight_limit(old_bridge_weight_limit);
		}
		const grund_t* gr = welt->lookup(get_pos());
		const bruecke_t *bridge = gr ? gr->find<bruecke_t>() : NULL;
		const tunnel_t *tunnel = gr ? gr->find<tunnel_t>() : NULL;
		const slope_t::type hang = gr ? gr->get_weg_hang() : slope_t::flat;

		if(hang != slope_t::flat) 
		{
			const uint slope_height = (hang & 7) ? 1 : 2;
			if(slope_height == 1)
			{
				if(bridge)
				{
					set_max_speed(min(desc->get_topspeed_gradient_1(), bridge->get_desc()->get_topspeed_gradient_1()));
				}
				else if(tunnel)
				{
					set_max_speed(min(desc->get_topspeed_gradient_1(), tunnel->get_desc()->get_topspeed_gradient_1()));
				}
				else
				{
					set_max_speed(desc->get_topspeed_gradient_1());
				}
			}
			else
			{
				if(bridge)
				{
					set_max_speed( min(desc->get_topspeed_gradient_2(), bridge->get_desc()->get_topspeed_gradient_2()));
				}
				else if(tunnel)
				{
					set_max_speed(min(desc->get_topspeed_gradient_2(), tunnel->get_desc()->get_topspeed_gradient_2()));
				}
				else
				{
					set_max_speed(desc->get_topspeed_gradient_2());
				}
			}
		}
		else
		{
			if(bridge)
				{
					set_max_speed(min(desc->get_topspeed(), bridge->get_desc()->get_topspeed()));
				}
			else if(tunnel)
				{
					set_max_speed(min(desc->get_topspeed(), tunnel->get_desc()->get_topspeed()));
				}
				else
				{
					set_max_speed(desc->get_topspeed());
				}
		}
 
		if(hat_gehweg() && desc->get_wtyp() == road_wt)
		{
			set_max_speed(min(get_max_speed(), 50));
		}
	}
}
