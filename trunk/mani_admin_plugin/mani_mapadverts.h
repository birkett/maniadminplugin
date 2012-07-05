//
// Mani Admin Plugin
//
// Copyright © 2009-2012 Giles Millward (Mani). All rights reserved.
//
// This file is part of ManiAdminPlugin.
//
// Mani Admin Plugin is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Mani Admin Plugin is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Mani Admin Plugin.  If not, see <http://www.gnu.org/licenses/>.
//


//



#ifndef MANI_MAPADVERTS_H
#define MANI_MAPADVERTS_H

class ManiMapAdverts
{

public:
	ManiMapAdverts();
	~ManiMapAdverts();

	void		Init(void);
	void		FreeMapAdverts(void);
	void		ClientActive(player_t *player_ptr);
	void		DumpCoords(player_t *player_ptr);

private:

	void	GetCoordList(KeyValues *kv_ptr, char *decal_name);

	struct decal_coord_t
	{
		float		x;
		float		y;
		float		z;
	};

	struct decal_name_t
	{
		char		name[64];
		int			index;
		decal_coord_t	*decal_coord_list;
		int				decal_coord_list_size;

	};

	decal_name_t	*decal_list;
	int				decal_list_size;

};

extern	ManiMapAdverts *gpManiMapAdverts;

#endif
