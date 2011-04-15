//
// Mani Admin Plugin
//
// Copyright © 2009-2011 Giles Millward (Mani). All rights reserved.
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



#ifndef MANI_AUTOMAP_H
#define MANI_AUTOMAP_H

class ManiAutoMap
{
public:
	ManiAutoMap();
	~ManiAutoMap();

	void		Load(void);
	void		Unload(void);
	void		LevelInit(void);
	void		GameFrame(void);
	void		SetupMapList(void);
	void		ResetTimeout(int seconds);
	void		SetMapOverride(bool enable_override);

private:

	int			ChooseMap(void);

	struct automap_id_t
	{
		char	map_name[64];
	};

	automap_id_t	*automap_list;
	int				automap_list_size;

	bool		set_next_map;

	time_t		trigger_time;
	bool		ignore_this_map;
};

extern	ManiAutoMap *gpManiAutoMap;

#endif
