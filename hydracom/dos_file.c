/*=============================================================================

                              HydraCom Version 1.00

                         A sample implementation of the
                   HYDRA Bi-Directional File Transfer Protocol

                             HydraCom was written by
                   Arjen G. Lentz, LENTZ SOFTWARE-DEVELOPMENT
                  COPYRIGHT (C) 1991-1993; ALL RIGHTS RESERVED

                       The HYDRA protocol was designed by
                 Arjen G. Lentz, LENTZ SOFTWARE-DEVELOPMENT and
                             Joaquim H. Homrighausen
                  COPYRIGHT (C) 1991-1993; ALL RIGHTS RESERVED


  Revision history:
  06 Sep 1991 - (AGL) First tryout
  .. ... .... - Internal development
  11 Jan 1993 - HydraCom version 1.00, Hydra revision 001 (01 Dec 1992)


  For complete details of the Hydra and HydraCom licensing restrictions,
  please refer to the license agreements which are published in their entirety
  in HYDRACOM.C and LICENSE.DOC, and also contained in the documentation file
  HYDRACOM.DOC

  Use of this file is subject to the restrictions contained in the Hydra and
  HydraCom licensing agreements. If you do not find the text of this agreement
  in any of the aforementioned files, or if you do not have these files, you
  should immediately contact LENTZ SOFTWARE-DEVELOPMENT and/or Joaquim
  Homrighausen at one of the addresses listed below. In no event should you
  proceed to use this file without having accepted the terms of the Hydra and
  HydraCom licensing agreements, or such other agreement as you are able to
  reach with LENTZ SOFTWARE-DEVELOMENT and Joaquim Homrighausen.


  Hydra protocol design and HydraCom driver:         Hydra protocol design:
  Arjen G. Lentz                                     Joaquim H. Homrighausen
  LENTZ SOFTWARE-DEVELOPMENT                         389, route d'Arlon
  Langegracht 7B                                     L-8011 Strassen
  3811 BT  Amersfoort                                Luxembourg
  The Netherlands
  FidoNet 2:283/512, AINEX-BBS +31-33-633916         FidoNet 2:270/17
  arjen_lentz@f512.n283.z2.fidonet.org               joho@ae.lu

  Please feel free to contact us at any time to share your comments about our
  software and/or licensing policies.

=============================================================================*/

#include <fcntl.h>
#include "hydracom.h"

int dos_sopen(char *pathname, byte create)
{
	int access;

	if (create == 0)
		access = O_RDONLY;
	else
		access = O_RDWR;
	if (create) {
		access |= O_CREAT;
		if (create == 2)
			access |= O_TRUNC;
	}

	return open(pathname, access, 0644);
}

int dos_close(int handle)
{
	return (close(handle));
}

long dos_seek(int handle, long offset, int fromwhere)
{
	return (lseek(handle, offset, fromwhere));
}

long dos_tell(int handle)
{
	return (lseek(handle, 0L, SEEK_CUR));
}

int dos_read(int handle, void *buf, word len)
{
	return ((int) read(handle, buf, len));
}

int dos_write(int handle, void *buf, word len)
{
	return ((int) write(handle, buf, len));
}

FILE *sfopen(char *name, char *mode, int shareflag)
{
	return (fopen(name, mode));
}
