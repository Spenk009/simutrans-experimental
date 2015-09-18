/*
 * Copyright (c) 1997 - 2001 Hj. Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include <string.h>
#include "../simdebug.h"
#include "../simtool.h"
#include "../gui/simwin.h"
#include "../simworld.h"

#include "../dataobj/translator.h"
#include "../network/network_cmd_ingame.h"

#include "../utils/cbuffer_t.h"
#include "../utils/sha1.h"
#include "../utils/simstring.h"


#include "password_frame.h"
#include "player_frame_t.h"

#define DIALOG_WIDTH (360)


password_frame_t::password_frame_t( player_t *player ) :
	gui_frame_t( translator::translate("Enter Password"), player )
{
	this->player = player;

	if(  !player->is_locked()  ||  (welt->get_active_player_nr()==1  &&  !welt->get_player(1)->is_locked())   ) {
		// allow to change name name
		tstrncpy( player_name_str, player->get_name(), lengthof(player_name_str) );
		player_name.set_text(player_name_str, lengthof(player_name_str));
		player_name.add_listener(this);
		player_name.set_pos(scr_coord(10,4));
		player_name.set_size(scr_size(DIALOG_WIDTH-10-10, D_BUTTON_HEIGHT));
		add_component(&player_name);
	}
	else {
		const_player_name.set_text( player->get_name() );
		const_player_name.set_pos(scr_coord(10,4));
		add_component(&const_player_name);
	}


	fnlabel.set_pos (scr_coord(10,4+D_BUTTON_HEIGHT+6));
	fnlabel.set_text( "Password" );
	add_component(&fnlabel);

	// Input box for password
	ibuf[0] = 0;
	password.set_text(ibuf, lengthof(ibuf) );
	password.add_listener(this);
	password.set_pos(scr_coord(75,4+D_BUTTON_HEIGHT+4));
	password.set_size(scr_size(DIALOG_WIDTH-75-10, D_BUTTON_HEIGHT));
	add_component(&password);
	set_focus( &password );

	set_windowsize(scr_size(DIALOG_WIDTH, 16+12+2*D_BUTTON_HEIGHT));
}




/**
 * This method is called if an action is triggered
 * @author Hj. Malthaner
 */
bool password_frame_t::action_triggered( gui_action_creator_t *comp, value_t p )
{
	if(comp == &password  &&  (ibuf[0]!=0  ||  p.i == 1)) {
		if (player->is_unlock_pending()) {
			// unlock already pending, do not do everything twice
			return true;
		}
		// Enter-Key pressed
		// test for matching password to unlock
		SHA1 sha1;
		size_t len = strlen( password.get_text() );
		sha1.Input( password.get_text(), len );
		pwd_hash_t hash;
		// remove hash to re-open slot if password is empty
		if(len>0) {
			hash.set(sha1);
		}
		// store the hash
		welt->store_player_password_hash( player->get_player_nr(), hash );

		if(  env_t::networkmode) {
			player->unlock(!player->is_locked(), true);
			// send hash to server: it will unlock player or change password
			nwc_auth_player_t *nwc = new nwc_auth_player_t(player->get_player_nr(), hash);
			network_send_server(nwc);
		}
		else {
			/* if current active player is player 1 and this is unlocked, he may reset passwords
			 * otherwise you need the valid previous password
			 */
			if(  !player->is_locked()  ||  (welt->get_active_player_nr()==1  &&  !welt->get_player(1)->is_locked())   ) {
				// set password
				player->access_password_hash() = hash;
				player->unlock(true, false);
			}
			else {
				player->check_unlock(hash);
			}
		}
	}

	if(  comp == &player_name  ) {
		// rename a player
		cbuffer_t buf;
		buf.printf( "p%u,%s", player->get_player_nr(), player_name.get_text() );
		tool_t *tool = create_tool( TOOL_RENAME | SIMPLE_TOOL );
		tool->set_default_param( buf );
		welt->set_tool( tool, player );
		// since init always returns false, it is safe to delete immediately
		delete tool;
	}

	if(  p.i==1  ) {
		// destroy window after enter is pressed
		destroy_win(this);
	}
	return true;
}
