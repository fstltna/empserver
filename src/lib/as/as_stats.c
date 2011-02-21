/*
 *  A* Search - A search library used in Empire to determine paths between
 *              objects.
 *  Copyright (C) 1990-1998 Phil Lapsley
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
 */

#include <config.h>

#include "as.h"

/*
 * Print statistics on algorithm performance to the file pointer "fp".
 */
void
as_stats(struct as_data *adp, FILE * fp)
{
    int i;
    int j;
    int total_q;
    int total_p;
    int total_h;
    size_t other;
    struct as_queue *qp;
    struct as_path *pp;
    struct as_hash *hp;

    fprintf(fp, "Statistics:\n");

    fprintf(fp, "queue lengths:\n");
    total_q = 0;
    total_h = 0;
    for (i = 0, qp = adp->head; qp; qp = qp->next)
	i++;
    fprintf(fp, "\tmain:\t%d\n", i);
    total_q += i;
    for (i = 0, qp = adp->tried; qp; qp = qp->next)
	i++;
    fprintf(fp, "\ttried:\t%d\n", i);
    total_q += i;
    for (i = 0, qp = adp->subsumed; qp; qp = qp->next)
	i++;
    fprintf(fp, "\tsubsumed:\t%d\n", i);
    total_q += i;
    for (i = 0, pp = adp->path; pp; pp = pp->next)
	i++;
    total_p = i;
    fprintf(fp, "path length: %d\n", total_p);
    fprintf(fp, "hash table statistics (size %d):\n", adp->hashsize);
    for (i = 0; i < adp->hashsize; i++) {
	for (j = 0, hp = adp->hashtab[i]; hp; hp = hp->next)
	    j++;
	fprintf(fp, "\t%d\t%d\n", i, j);
	total_h += j;
    }
    fprintf(fp, "\ttotal\t%d\n", total_h);
    fprintf(fp, "approximate memory usage (bytes):\n");
    fprintf(fp, "\tqueues\t%d\n",
	    (int)(total_q * sizeof(struct as_queue)));
    fprintf(fp, "\tnodes\t%d\n", (int)(total_q * sizeof(struct as_node)));
    fprintf(fp, "\tpath\t%d\n", (int)(total_p * sizeof(struct as_path)));
    fprintf(fp, "\thash ents\t%d\n",
	    (int)(total_h * sizeof(struct as_hash)));
    other = sizeof(struct as_data);
    other += adp->maxneighbors * sizeof(struct as_coord);
    other += (adp->maxneighbors + 1) * sizeof(struct as_node *);
    other += adp->hashsize * sizeof(struct as_hash *);
    fprintf(fp, "\tother\t%d\n", (int)other);
    fprintf(fp, "\ttotal\t%d\n",
	    (int)(total_q * sizeof(struct as_queue) +
		  total_q * sizeof(struct as_node) +
		  total_p * sizeof(struct as_path) +
		  total_h * sizeof(struct as_hash) +
		  other));
}