/*
 *  Empire - A multi-player, client/server Internet based war game.
 *  Copyright (C) 1986-2006, Dave Pare, Jeff Bailey, Thomas Ruschak,
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
 *  tfact.c: return tech fact given multiplier
 * 
 *  Known contributors to this file:
 *     Yannick Trembley
 */

#include <config.h>

#include "misc.h"
#include "nat.h"
#include "file.h"
#include "optlist.h"

double
tfact(natid cn, double mult)
{
    double tlev;
    struct natstr *np;

    np = getnatp(cn);
    tlev = np->nat_level[NAT_TLEV];
    tlev = (50.0 + tlev) / (200.0 + tlev);
    return mult * tlev;
}

double
tfactfire(natid cn, double mult)
{
    double tlev;
    struct natstr *np;

    np = getnatp(cn);
    tlev = np->nat_level[NAT_TLEV];
    tlev = (50.0 + tlev) / (200.0 + tlev);
    return mult * tlev * fire_range_factor;
}

double
techfact(int level, double mult)
{
    return mult * ((50.0 + level) / (200.0 + level));
}
