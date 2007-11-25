/*
 *  Empire - A multi-player, client/server Internet based war game.
 *  Copyright (C) 1986-2007, Dave Pare, Jeff Bailey, Thomas Ruschak,
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
 *  player.h: Definitions for player information (threads)
 * 
 *  Known contributors to this file:
 *     Dave Pare, 1994
 *     Doug Hay, 1998
 */

#ifndef PLAYER_H
#define PLAYER_H

#include <time.h>
#include "empthread.h"
#include "misc.h"
#include "queue.h"
#include "types.h"

	/* nstat values */
#define VIS		bit(0)
#define SANCT		(bit(1) | VIS)
#define NORM		(bit(2) | VIS)
#define GOD		(bit(3) | NORM | VIS)
#define	CAP		bit(6)
#define	MONEY		bit(7)

struct player {
    struct emp_qelem queue;
    empth_t *proc;
    char hostaddr[32];
    char hostname[512];		/* may be empty */
    char client[128];		/* may be empty */
    char userid[32];		/* may be empty */
    int authenticated;
    natid cnum;
    int state;
    int flags;
    struct cmndstr *command;	/* currently executing command */
    struct iop *iop;
    char combuf[1024];		/* command input buffer, UTF-8 */
    char *argp[128];		/* arguments, ASCII, valid if command */
    char *condarg;		/* conditional, ASCII, valid if command */
    char *comtail[128];		/* start of args in combuf[] */
    time_t lasttime;		/* when minleft was last debited */
    int ncomstat;
    int minleft;
    int btused;
    int god;
    int owner;
    int nstat;
    int simulation;		/* e.g. budget command */
    double dolcost;
    int broke;
    time_t curup;		/* when last input was received */
    int aborted;		/* interrupt cookie received? */
    int eof;			/* EOF (cookie or real) received? */
    int curid;			/* for pr, cur. line's id, -1 none */
    char *map;			/* pointer to in-mem map */
    char *bmap;			/* pointer to in-mem bmap */
};

#define PS_INIT		0
#define PS_PLAYING	1
#define PS_SHUTDOWN	2

/* player flags */
enum {
    PF_UTF8 = bit(0)			/* client wants UTF-8 */
};

#endif
