
/*****************************************************************************\
 *                                                                           *
 * Soothsayer, an extensible predictive text entry system                    *
 * ------------------------------------------------------                    *
 *                                                                           *
 * Copyright (C) 2004  Matteo Vescovi <matteo.vescovi@yahoo.co.uk>           *
 *                                                                           *
 * This program is free software; you can redistribute it and/or             *
 * modify it under the terms of the GNU General Public License               *
 * as published by the Free Software Foundation; either version 2            *
 * of the License, or (at your option) any later version.                    *
 *                                                                           *
 * This program is distributed in the hope that it will be useful,           *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of            *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
 * GNU General Public License for more details.                              *
 *                                                                           *
 * You should have received a copy of the GNU General Public License         *
 * along with this program; if not, write to the Free Software               *
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.*
 *                                                                           *
\*****************************************************************************/


#include <iostream>
#include "plugins/dummyPlugin.h"
#include "core/contextTracker.h"
#include <vector>
#include <string>

int main()
{
	Profile mockProfile(0);
	ContextTracker contextTracker(&mockProfile);
	DummyPlugin dummyPlugin(&mockProfile, &contextTracker);
	Plugin* pluginPtr = &dummyPlugin;
	

	std::cout << dummyPlugin.getName()
		  << " test driver program"
		  << std::endl;

	dummyPlugin.learn();
	dummyPlugin.extract();
	dummyPlugin.train();

	std::cout << dummyPlugin.predict(100);

	std::cout << "Now testing polymorphic behaviour..." << std::endl;

	pluginPtr->learn();
	pluginPtr->extract();
	pluginPtr->train();
	std::cout << pluginPtr->predict(100);

	std::cout << "Testing name and description functions..." << std::endl
		  << "Name: " << pluginPtr->getName() << std::endl
		  << "Short description: " << pluginPtr->getShortDescription()
		  << std::endl
		  << "Long description: " << pluginPtr->getLongDescription()
		  << std::endl;

	return 0;
}

