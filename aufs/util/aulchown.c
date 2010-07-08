
/*
 * Copyright (C) 2005-2009 Junjiro Okajima
 *
 * This program, aufs is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

/*
 * While this tool should be atomic, I choose loose/rough way.
 * cf. auplink and aufs.5
 * $Id: aulchown.c,v 1.3 2009/01/26 06:24:45 sfjro Exp $
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>

int main(int argc, char *argv[])
{
	int err, nerr;
	struct stat st;

	nerr = 0;
	while (*++argv) {
		err = lstat(*argv, &st);
		if (!err && S_ISLNK(st.st_mode))
			err = lchown(*argv, st.st_uid, st.st_gid);
		if (!err)
			continue;
		perror(*argv);
		nerr++;
	}
	return nerr;
}
