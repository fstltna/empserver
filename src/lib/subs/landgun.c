/*
 *  Empire - A multi-player, client/server Internet based war game.
 *  Copyright (C) 1986-2008, Dave Pare, Jeff Bailey, Thomas Ruschak,
 *                           Ken Stevens, Steve McClure
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  ---
 *
 *  See files README, COPYING and CREDITS in the root of the source
 *  tree for related information and legal notices.  It is expected
 *  that future projects/authors will amend these files as needed.
 *
 *  ---
 *
 *  landgun.c: Return values for land and ship gun firing damages
 * 
 *  Known contributors to this file:
 *     Markus Armbruster, 2006
 */

#include <config.h>

#include "damage.h"
#include "file.h"
#include "nat.h"
#include "optlist.h"
#include "prototypes.h"
#include "sect.h"
#include "ship.h"

double
fortgun(int effic, int guns)
{
    double d;
    double g = MIN(guns, 7);

    d = (random() % 30 + 20.0) * (g / 7.0);
    d *= effic / 100.0;
    return d;
}

double
seagun(int effic, int guns)
{
    double d;

    d = 0.0;
    while (guns--)
	d += 10.0 + random() % 6;
    d *= effic * 0.01;
    return d;
}

double
landunitgun(int effic, int shots, int guns, int ammo, int shells)
{
    double d = 0.0;

    shots = MIN(shots, guns);
    while (shots-- > 0)
	d += 5.0 + random() % 6;
    d *= effic * 0.01;
    if (shells < ammo && ammo != 0)
	d *= (double)shells / (double)ammo;
    return d;
}

/*
 * Fire from fortress SP.
 * Use ammo, resupply if necessary.
 * Return damage if the fortress fires, else -1.
 */
int
fort_fire(struct sctstr *sp)
{
    int guns = sp->sct_item[I_GUN];
    int shells;

    if (sp->sct_type != SCT_FORTR || sp->sct_effic < FORTEFF)
	return -1;
    if (sp->sct_item[I_MILIT] < 5 || guns == 0)
	return -1;
    shells = sp->sct_item[I_SHELL];
    shells += supply_commod(sp->sct_own, sp->sct_x, sp->sct_y,
			    I_SHELL, 1 - shells);
    if (shells == 0)
	return -1;
    sp->sct_item[I_SHELL] = shells - 1;
    return (int)fortgun(sp->sct_effic, guns);
}

/*
 * Fire from ship SP.
 * Use ammo, resupply if necessary.
 * Return damage if the ship fires, else -1.
 */
int
shp_fire(struct shpstr *sp)
{
    int guns, shells;

    if (sp->shp_effic < 60)
	return -1;
    guns = sp->shp_glim;
    guns = MIN(guns, sp->shp_item[I_GUN]);
    guns = MIN(guns, (sp->shp_item[I_MILIT] + 1) / 2);
    if (guns == 0)
	return -1;
    shells = sp->shp_item[I_SHELL];
    shells += supply_commod(sp->shp_own, sp->shp_x, sp->shp_y,
                           I_SHELL, (guns + 1) / 2 - shells);
    guns = MIN(guns, shells * 2);
    if (guns == 0)
       return -1;
    sp->shp_item[I_SHELL] = shells - (guns + 1) / 2;
    return (int)seagun(sp->shp_effic, guns);
}

/*
 * Drop depth-charges from ship SP.
 * Use ammo, resupply if necessary.
 * Return damage if the ship drops depth-charges, else -1.
 */
int
shp_dchrg(struct shpstr *sp)
{
    int shells;

    if (sp->shp_effic < 60 || (mchr[sp->shp_type].m_flags & M_DCH) == 0)
	return -1;
    if (sp->shp_item[I_MILIT] == 0)
	return -1;
    shells = sp->shp_item[I_SHELL];
    shells += supply_commod(sp->shp_own, sp->shp_x, sp->shp_y,
                           I_SHELL, 2 - shells);
    if (shells < 2)
       return -1;
    sp->shp_item[I_SHELL] = shells - 2;
    return (int)seagun(sp->shp_effic, 3);
}

/*
 * Fire torpedo from ship SP.
 * Use ammo and mobility, resupply if necessary.
 * Return damage if the ship fires, else -1.
 */
int
shp_torp(struct shpstr *sp, int usemob)
{
    int shells;

    if (sp->shp_effic < 60 || (mchr[sp->shp_type].m_flags & M_TORP) == 0)
	return -1;
    if (sp->shp_item[I_MILIT] == 0 || sp->shp_item[I_GUN] == 0)
	return -1;
    if (usemob && sp->shp_mobil <= 0)
	return -1;
    shells = sp->shp_item[I_SHELL];
    shells += supply_commod(sp->shp_own, sp->shp_x, sp->shp_y,
                           I_SHELL, SHP_TORP_SHELLS - shells);
    if (shells < SHP_TORP_SHELLS)
       return -1;
    sp->shp_item[I_SHELL] = shells - SHP_TORP_SHELLS;
    if (usemob)
	sp->shp_mobil -= (int)shp_mobcost(sp) / 2.0;
    return TORP_DAMAGE();
}

/*
 * Return effective firing range for range factor RNG at tech TLEV.
 */
double
effrange(int rng, double tlev)
{
    /* FIXME don't truncate TLEV */
    return techfact((int)tlev, rng / 2.0);
}

/*
 * Return torpedo range for ship SP.
 */
double
torprange(struct shpstr *sp)
{
    return effrange(sp->shp_frnge * 2, sp->shp_tech)
	* sp->shp_effic / 100.0;
}

/*
 * Return firing range for sector SP.
 */
double
fortrange(struct sctstr *sp)
{
    struct natstr *np = getnatp(sp->sct_own);
    double rng;

    if (sp->sct_type != SCT_FORTR || sp->sct_effic < FORTEFF)
	return -1.0;

    rng = effrange(14.0 * fire_range_factor, np->nat_level[NAT_TLEV]);
    if (sp->sct_effic > 59)
	rng++;
    return rng;
}

int
roundrange(double r)
{
    return roundavg(r);
}
