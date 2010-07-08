//
// Mani Admin Plugin
//
// Copyright (c) 2009 Giles Millward (Mani). All rights reserved.
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




#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "interface.h"
#include "filesystem.h"
#include "engine/iserverplugin.h"
#include "iplayerinfo.h"
#include "eiface.h"
#include "igameevents.h"
#include "mrecipientfilter.h" 
#include "bitbuf.h"
#include "engine/IEngineSound.h"
#include "inetchannelinfo.h"
#include "networkstringtabledefs.h"
#include "mani_main.h"

extern	IVEngineServer	*engine; // helper functions (messaging clients, loading content, making entities, running commands, etc)

static	float mani_timers[20];
static	int	timer_index = 0;

//---------------------------------------------------------------------------------
// Purpose: Get the next timer index to use
//---------------------------------------------------------------------------------
int		ManiGetTimer(void)
{
	int temp_index;

	mani_timers[timer_index] = engine->Time();
	temp_index = timer_index;
	timer_index ++;
	if (timer_index == 20)
	{
		timer_index = 0;
	}

	return temp_index;
}

//---------------------------------------------------------------------------------
// Purpose: Get the timer duration
//---------------------------------------------------------------------------------
float	ManiGetTimerDuration(int index)
{
	return (engine->Time() - mani_timers[index]);
}
