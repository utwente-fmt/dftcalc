#include "ConsoleWriter.h"

#include <iostream>

#ifdef WIN32
#include <windows.h>
const ConsoleWriter::Color ConsoleWriter::Color::Error  (FOREGROUND_RED );
const ConsoleWriter::Color ConsoleWriter::Color::Warning(FOREGROUND_RED  | FOREGROUND_GREEN);
const ConsoleWriter::Color ConsoleWriter::Color::Notify (FOREGROUND_BLUE | FOREGROUND_INTENSITY);
const ConsoleWriter::Color ConsoleWriter::Color::Proper (FOREGROUND_GREEN | FOREGROUND_INTENSITY);
const ConsoleWriter::Color ConsoleWriter::Color::Message(0);
const ConsoleWriter::Color ConsoleWriter::Color::Reset  (0);
#else
#include <cstdio>
#include <cstdlib>
const ConsoleWriter::Color ConsoleWriter::Color::Error  (31);
const ConsoleWriter::Color ConsoleWriter::Color::Warning(33);
const ConsoleWriter::Color ConsoleWriter::Color::Notify (134);
const ConsoleWriter::Color ConsoleWriter::Color::Proper (132);
const ConsoleWriter::Color ConsoleWriter::Color::Message(0);
const ConsoleWriter::Color ConsoleWriter::Color::Reset  (0);
#endif

ConsoleWriter::ConsoleWriter(std::ostream& out):
	FileWriter(),
	out(out) {
		
	if(out==std::cout) {
		kindOfStream = 1;
	} else if(out==std::cerr) {
		kindOfStream = 2;
	} else {
		kindOfStream = 0;
	}
	
	if(kindOfStream>0) {
		#ifdef WIN32
			CONSOLE_SCREEN_BUFFER_INFO out;
			HANDLE hConsole = GetStdHandle(kindOfStream==1?STD_OUTPUT_HANDLE:STD_ERROR_HANDLE);
			GetConsoleScreenBufferInfo(hConsole,&out);
			colorStack.push(Color(out.wAttributes));
		#else
			colorStack.push(ConsoleWriter::Color::Reset);
		#endif
	}
}

ConsoleWriter& ConsoleWriter::operator<<(ConsoleWriter::Color color) {
	#ifdef WIN32
		if(sss.size() == 1 && kindOfStream > 0) {
			HANDLE hConsole = GetStdHandle(kindOfStream==1?STD_OUTPUT_HANDLE:STD_ERROR_HANDLE);
			if(color.getColorCode()==0) {
				SetConsoleTextAttribute(hConsole, colorStack.top().getColorCode());
			} else {
				SetConsoleTextAttribute(hConsole, color.getColorCode());
			}
		}
	#else
		out << "\033[" << (color.getColorCode()/100) << ";" << (color.getColorCode()%100) << "m";
		out << "\033[" << (color.getColorCode()/100) << ";" << (color.getColorCode()%100) << "m";
	#endif
	return *this;
}
