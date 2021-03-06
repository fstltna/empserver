/*
 *  Empire - A multi-player, client/server Internet based war game.
 *  Copyright (C) 1986-2013, Dave Pare, Jeff Bailey, Thomas Ruschak,
 *                Ken Stevens, Steve McClure, Markus Armbruster
 *
 *  Empire is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  ---
 *
 *  See files README, COPYING and CREDITS in the root of the source
 *  tree for related information and legal notices.  It is expected
 *  that future projects/authors will amend these files as needed.
 *
 *  ---
 *
 *  foll.c: Set leader of a set of ships
 *
 *  Known contributors to this file:
 *     Robert Forsman
 */

#include <config.h>

#include "commands.h"
#include "optlist.h"
#include "ship.h"

int
foll(void)
{
    struct shpstr ship;
    char *cp;
    int good, leader, count = 0;
    coord x, y;
    struct nstr_item nstr;
    char buf[1024];

    if (!opt_SAIL) {
	pr("The SAIL option is not enabled, so this command is not valid.\n");
	return RET_FAIL;
    }
    if (!snxtitem(&nstr, EF_SHIP, player->argp[1], NULL))
	return RET_SYN;
    cp = getstarg(player->argp[2], "leader? ", buf);
    if (!cp)
	cp = "";
    good = sscanf(cp, "%d", &leader);
    if (!good)
	return RET_SYN;
    getship(leader, &ship);
    if (relations_with(ship.shp_own, player->cnum) < FRIENDLY) {
	pr("That ship won't let you follow.\n");
	return RET_FAIL;
    }
    x = ship.shp_x;
    y = ship.shp_y;
    while (nxtitem(&nstr, &ship)) {
	if (!player->owner)
	    continue;
	if (ship.shp_x != x || ship.shp_y != y) {
	    pr("Ship #%d not in same sector as #%d\n",
	       ship.shp_uid, leader);
	    continue;
	}
	if (ship.shp_uid == leader) {
	    pr("Ship #%d can't follow itself!\n", leader);
	    continue;
	}
	if ((ship.shp_autonav & AN_AUTONAV)
	    && !(ship.shp_autonav & AN_STANDBY)) {
	    pr("Ship #%d has other orders!\n", ship.shp_uid);
	    continue;
	}
	count++;
	ship.shp_mission = 0;
	*ship.shp_path = 'f';
	ship.shp_path[1] = 0;
	ship.shp_follow = leader;
	pr("Ship #%d follows #%d.\n", ship.shp_uid, leader);
	putship(ship.shp_uid, &ship);
    }
    if (count == 0) {
	if (player->argp[1])
	    pr("%s: No ship(s)\n", player->argp[1]);
	else
	    pr("%s: No ship(s)\n", "");
	return RET_FAIL;
    } else
	pr("%d ship%s\n", count, splur(count));
    return RET_OK;
}
