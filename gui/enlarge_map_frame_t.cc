/*
 * Dialogue to increase map size.
 *
 * Gerd Wachsmuth
 *
 * October 2008
 */

#include <string.h>

#include "enlarge_map_frame_t.h"
#include "karte.h"
#include "messagebox.h"

#include "../simdebug.h"
#include "../simworld.h"
#include "../gui/simwin.h"
#include "../display/simimg.h"
#include "../utils/simrandom.h"
#include "../simintr.h"
#include "welt.h"

#include "../dataobj/environment.h"
#include "../dataobj/settings.h"
#include "../dataobj/translator.h"

// just for their structure size ...
#include "../boden/wege/schiene.h"
#include "../obj/baum.h"
#include "../simcity.h"
#include "../vehicle/simvehicle.h"
#include "../player/simplay.h"
#include "../simconvoi.h"

#include "../simcolor.h"

#include "../display/simgraph.h"

#include "../utils/simstring.h"

#define START_HEIGHT (28)

#define LEFT_ARROW (110)
#define RIGHT_ARROW (160)

#define RIGHT_COLUMN (185)
#define RIGHT_COLUMN_WIDTH (60)



koord enlarge_map_frame_t::koord_from_rotation(settings_t const* const sets, sint16 const x, sint16 const y, sint16 const w, sint16 const h)
{
	koord offset( sets->get_origin_x(), sets->get_origin_y() );
	switch( sets->get_rotation() ) {
		default:
		case 0: return offset+koord(x,y);
		case 1: return offset+koord(y,w-x);
		case 2: return offset+koord(w-x,h-y);
		case 3: return offset+koord(h-y,x);
	}
}


enlarge_map_frame_t::enlarge_map_frame_t() :
	gui_frame_t( translator::translate("enlarge map") ),
	sets(new settings_t(welt->get_settings())), // Make a copy.
	memory(memory_str)
{
	sets->set_size_x(welt->get_size().x);
	sets->set_size_y(welt->get_size().y);
	number_of_big_cities  = 0;
	number_of_clusters = 0;
	cluster_size = env_t::cluster_size;
	
	changed_number_of_towns = false;
	int intTopOfButton = 24;

	memory.set_pos( scr_coord(10,intTopOfButton) );
	add_component( &memory );

	inp_x_size.set_pos(scr_coord(LEFT_ARROW,intTopOfButton) );
	inp_x_size.set_size(scr_size(RIGHT_ARROW-LEFT_ARROW+10, 12));
	inp_x_size.add_listener(this);
	inp_x_size.set_value( sets->get_size_x() );
	inp_x_size.set_limits( welt->get_size().x, min(welt_gui_t::max_map_dimension_fixed, welt_gui_t::max_map_dimension_numerator/sets->get_size_y()) );
	inp_x_size.set_increment_mode( sets->get_size_x()>=512 ? 128 : 64 );
	inp_x_size.wrap_mode( false );
	add_component( &inp_x_size );
	intTopOfButton += 12;

	inp_y_size.set_pos(scr_coord(LEFT_ARROW,intTopOfButton) );
	inp_y_size.set_size(scr_size(RIGHT_ARROW-LEFT_ARROW+10, 12));
	inp_y_size.add_listener(this);
	inp_y_size.set_limits( welt->get_size().y, min(welt_gui_t::max_map_dimension_fixed, welt_gui_t::max_map_dimension_numerator /sets->get_size_x()) );
	inp_y_size.set_value( sets->get_size_y() );
	inp_y_size.set_increment_mode( sets->get_size_y()>=512 ? 128 : 64 );
	inp_y_size.wrap_mode( false );
	add_component( &inp_y_size );

	// city stuff
	intTopOfButton = 64+10;
	inp_number_of_towns.set_pos(scr_coord(RIGHT_COLUMN,intTopOfButton) );
	inp_number_of_towns.set_size(scr_size(RIGHT_COLUMN_WIDTH, 12));
	inp_number_of_towns.add_listener(this);
	inp_number_of_towns.set_limits(0,999);
	inp_number_of_towns.set_value(0);
	add_component( &inp_number_of_towns );
	intTopOfButton += 12;

	inp_number_of_big_cities.set_pos(scr_coord(RIGHT_COLUMN,intTopOfButton) );
	inp_number_of_big_cities.set_size(scr_size(RIGHT_COLUMN_WIDTH, 12));
	inp_number_of_big_cities.add_listener(this);
	inp_number_of_big_cities.set_limits(0,0);
	inp_number_of_big_cities.set_value(0);
	add_component( &inp_number_of_big_cities );
	intTopOfButton += 12;

	inp_number_of_clusters.set_pos(scr_coord(RIGHT_COLUMN,intTopOfButton) );
	inp_number_of_clusters.set_size(scr_size(RIGHT_COLUMN_WIDTH, 12));
	inp_number_of_clusters.add_listener(this);
	inp_number_of_clusters.set_limits(0,sets->get_city_count()/3 );
	inp_number_of_clusters.set_value(number_of_clusters);
	add_component( &inp_number_of_clusters );
	intTopOfButton += 12;

	inp_cluster_size.set_pos(scr_coord(RIGHT_COLUMN,intTopOfButton) );
	inp_cluster_size.set_size(scr_size(RIGHT_COLUMN_WIDTH, 12));
	inp_cluster_size.add_listener(this);
	inp_cluster_size.set_limits(1,9999);
	inp_cluster_size.set_value(cluster_size);
	add_component( &inp_cluster_size );
	intTopOfButton += 12;

	inp_town_size.set_pos(scr_coord(RIGHT_COLUMN,intTopOfButton) );
	inp_town_size.set_size(scr_size(RIGHT_COLUMN_WIDTH, 12));
	inp_town_size.add_listener(this);
	inp_town_size.set_limits(0,999999);
	inp_town_size.set_increment_mode(50);
	inp_town_size.set_value( sets->get_mean_einwohnerzahl() );
	add_component( &inp_town_size );
	intTopOfButton += 12+5;

	// start game
	intTopOfButton += 5;
	start_button.init( button_t::roundbox, "enlarge map", scr_coord(10, intTopOfButton), scr_size(240, 14) );
	start_button.add_listener( this );
	add_component( &start_button );

	set_windowsize( scr_size(260, intTopOfButton+14+8+16) );

	update_preview();
}


enlarge_map_frame_t::~enlarge_map_frame_t()
{
	delete sets;
}


/**
 * This method is called if an action is triggered
 * @author Hj. Malthaner
 */
bool enlarge_map_frame_t::action_triggered( gui_action_creator_t *comp,value_t v)
{
	if(comp==&inp_x_size) {
		sets->set_size_x( v.i );
		inp_x_size.set_increment_mode( v.i>=64 ? (v.i>=512 ? 128 : 64) : 8 );
		inp_y_size.set_limits( welt->get_size().y, min(welt_gui_t::max_map_dimension_fixed, welt_gui_t::max_map_dimension_numerator/sets->get_size_x()) );
		update_preview();
	}
	else if(comp==&inp_y_size) {
		sets->set_size_y( v.i );
		inp_y_size.set_increment_mode( v.i>=64 ? (v.i>=512 ? 128 : 64) : 8 );
		inp_x_size.set_limits( welt->get_size().x, min(welt_gui_t::max_map_dimension_fixed, welt_gui_t::max_map_dimension_numerator/sets->get_size_y()) );
		update_preview();
	}
	else if(comp==&inp_number_of_towns) {
		sets->set_city_count( v.i );
		if (v.i == 0) {
			number_of_big_cities = 0;
			inp_number_of_big_cities.set_limits(0,0);
			inp_number_of_big_cities.set_value(0);
		}
		else {
			inp_number_of_big_cities.set_limits(0, v.i); 
		}

		if ( number_of_big_cities > unsigned(v.i)) {
			number_of_big_cities= v.i;
			inp_number_of_big_cities.set_value( number_of_big_cities );
		}
	}
	else if(comp==&inp_number_of_big_cities) {
		number_of_big_cities = v.i;
	}
	else if(comp==&inp_town_size) {
		sets->set_mean_einwohnerzahl( v.i );
	}
	else if(comp==&start_button) {
		// since soon those are invalid
		intr_refresh_display( true );
		//Quick and Ugly Hack: we don't want change main env_t
		uint32 saved_number_of_big_cities = env_t::number_of_big_cities; env_t::number_of_big_cities = number_of_big_cities;
		uint32 saved_number_of_clusters  = env_t::number_of_clusters; env_t::number_of_clusters = number_of_clusters;
		welt->enlarge_map(sets, NULL);
		env_t::number_of_big_cities = saved_number_of_big_cities;
		env_t::number_of_clusters = saved_number_of_clusters; 		
	}
	else {
		return false;
	}
	return true;
}


void enlarge_map_frame_t::draw(scr_coord pos, scr_size size)
{
	while (welt->get_settings().get_rotation() != sets->get_rotation()) {
		// map was rotated while we are active ... => rotate too!
		sets->rotate90();
		sets->set_groesse( sets->get_size_y(), sets->get_size_x() );
		update_preview();
	}

	gui_frame_t::draw(pos, size);

	int x = pos.x+10;
	int y = pos.y+4+16;

	display_ddd_box_clip(x+173, y, preview_size+2, preview_size+2, MN_GREY0,MN_GREY4);
	display_array_wh(x+174, y+1, preview_size, preview_size, karte);

	y = pos.y+64+10+16;
	display_proportional_clip(x, y, translator::translate("5WORLD_CHOOSE"), ALIGN_LEFT, SYSCOL_TEXT, true);
	y += 12;
	display_proportional_clip(x, y, translator::translate("Number of big cities"), ALIGN_LEFT, SYSCOL_TEXT, true);
	y += 12;
	display_proportional_clip(x, y, translator::translate("Number of clusters"), ALIGN_LEFT, SYSCOL_TEXT, true);
	y += 12;
	display_proportional_clip(x, y, translator::translate("Cluster size"), ALIGN_LEFT, SYSCOL_TEXT, true);
	y += 12;
	display_proportional_clip(x, y, translator::translate("Median Citizen per town"), ALIGN_LEFT, SYSCOL_TEXT, true);
	y += 12+5;

	display_ddd_box_clip(x, y, 240, 0, MN_GREY0, MN_GREY4);
}


/**
 * Calculate the new Map-Preview. Initialize the new RNG!
 * @author Hj. Malthaner
 */
void enlarge_map_frame_t::update_preview()
{
	// reset noise seed
	setsimrand(0xFFFFFFFF, welt->get_settings().get_map_number());

	// "welt" still knows the old size. The new size is saved in "sets".
	sint16 old_x = welt->get_size().x;
	sint16 old_y = welt->get_size().y;
	sint16 pre_x = min(sets->get_size_x(), preview_size);
	sint16 pre_y = min(sets->get_size_y(), preview_size);

	const int mx = sets->get_size_x()/pre_x;
	const int my = sets->get_size_y()/pre_y;
	const sint32 map_size = max(sets->get_size_y(), sets->get_size_x());


	for(  int j=0;  j<pre_y;  j++  ) {
		for(  int i=0;  i<pre_x;  i++  ) {
			COLOR_VAL color;
			koord pos(i*mx,j*my);

			if(  pos.x<=old_x  &&  pos.y<=old_y  ){
				if(  i==(old_x/mx)  ||  j==(old_y/my)  ){
					// border
					color = COL_WHITE;
				}
				else {
					const sint16 height = welt->lookup_hgt( pos );
					color = reliefkarte_t::calc_hoehe_farbe(height, sets->get_groundwater());
				}
			}
			else {
				// new part
				const sint16 height = karte_t::perlin_hoehe(sets, pos, koord(old_x,old_y), map_size );
				color = reliefkarte_t::calc_hoehe_farbe(height, sets->get_groundwater());
			}
			karte[j*preview_size+i] = color;
		}
	}
	for(  int j=0;  j<preview_size;  j++  ) {
		for(  int i=(j<pre_y ? pre_x : 0);  i<preview_size;   i++  ) {
			karte[j*preview_size+i] = COL_GREY1;
		}
	}
	sets->heightfield = "";

	if(!changed_number_of_towns){// Interpolate number of towns.
		sint32 new_area = sets->get_size_x() * sets->get_size_y();
		sint32 old_area = old_x * old_y;
		sint32 const towns = welt->get_settings().get_city_count();
		const sint32 new_towns = towns * new_area / old_area - towns;
		sets->set_city_count( new_towns );
		inp_number_of_towns.set_value(abs(sets->get_city_count()) );
		if (new_towns != 0 ) {
			number_of_big_cities = 1;
			inp_number_of_big_cities.set_value(1);
			inp_number_of_big_cities.set_limits(1, new_towns); 
		}
	}

	// guess the new memory needed
	const uint sx = sets->get_size_x();
	const uint sy = sets->get_size_y();
	const sint32 memory = (
		sizeof(karte_t) +
		sizeof(player_t) * 8 +
		sizeof(convoi_t) * 1000 +
		(sizeof(schiene_t) + sizeof(vehicle_t)) * 10 * (sx + sy) +
		sizeof(stadt_t) * sets->get_city_count() +
		(
			sizeof(grund_t) +
			sizeof(planquadrat_t) +
			sizeof(baum_t) * 2 +
			sizeof(void*) * 4
		) * sx * sy
	) / (1024 * 1024);
	sprintf(memory_str, translator::translate("Size (%d MB):"), memory);

}
