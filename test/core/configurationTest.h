
/*********************************************************
 *  Soothsayer, an extensible predictive text entry system
 *  ------------------------------------------------------
 *
 *  Copyright (C) 2008  Matteo Vescovi <matteo.vescovi@yahoo.co.uk>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
                                                                             *
                                                                **********(*)*/

#ifndef SOOTH_CONFIGURATIONTEST
#define SOOTH_CONFIGURATIONTEST

#include <cppunit/extensions/HelperMacros.h>

#include "core/configuration.h"

class ConfigurationTest : public CppUnit::TestFixture { 
public:
    void setUp();
    void tearDown();

    void test_get_set();

private:
    Configuration* configuration;

    CPPUNIT_TEST_SUITE( ConfigurationTest  );
    CPPUNIT_TEST( test_get_set             );
    CPPUNIT_TEST_SUITE_END();
};

#endif // SOOTH_CONFIGURATIONTEST
