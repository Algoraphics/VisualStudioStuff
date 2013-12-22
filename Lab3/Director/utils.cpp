// Gabriel Hope, ghope@wustl.edu
// Kevin Kieselbach, kevin.kieselbach@wustl.edu
// Ethan Rabb, ethanrabb@go.wustl.edu
// 
// Trim and strip extension function definitions.
//

#include "stdafx.h"
#include "utils.h"

/*
 * Trim whitespace from the front of a string.
 */
string& trim(string& s)
{
	size_t n = s.find_first_not_of(" \t");
	if (n == string::npos)
	{
		s = "";
	}
	else
	{
		s = s.substr(n);
	}

	return s;
}

/*
 * Trim extension from the end of a string.
 */
string& stripExt(string& s)
{
	size_t n = s.find_last_of(".");
	if (n == string::npos)
	{
		return s;
	}
	else
	{
		s = s.substr(0, n);
	}

	return s;
}