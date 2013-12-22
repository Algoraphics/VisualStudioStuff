// Gabriel Hope, ghope@wustl.edu
// Kevin Kieselbach, kevin.kieselbach@wustl.edu
// Ethan Rabb, ethanrabb@go.wustl.edu
// 
// DialogueLine struct.
//

#pragma once

#include <string>

using namespace std;

/* 
 * Represents a line of dialogue in a play.
 */
struct DialogueLine
{
	unsigned lineNumber;
	string speaker;
	string text;
	DialogueLine(unsigned lineNumber_, string speaker_, string text_);
};