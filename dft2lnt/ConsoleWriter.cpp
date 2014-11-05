/*
 * CompilerContext.cpp
 * 
 * Part of a general library.
 * 
 * @author Freark van der Berg
 */

#include "ConsoleWriter.h"

#include <iostream>

#ifdef WIN32
#include <windows.h>
const ConsoleWriter::Color ConsoleWriter::Color::Error  (FOREGROUND_RED   );
const ConsoleWriter::Color ConsoleWriter::Color::Warning(FOREGROUND_RED   | FOREGROUND_GREEN);
const ConsoleWriter::Color ConsoleWriter::Color::Notify (FOREGROUND_BLUE  | FOREGROUND_INTENSITY);
const ConsoleWriter::Color ConsoleWriter::Color::Notify2(FOREGROUND_RED   | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
const ConsoleWriter::Color ConsoleWriter::Color::NotifyH(FOREGROUND_RED   | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
const ConsoleWriter::Color ConsoleWriter::Color::Action (FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
const ConsoleWriter::Color ConsoleWriter::Color::Proper (FOREGROUND_GREEN | FOREGROUND_INTENSITY);
const ConsoleWriter::Color ConsoleWriter::Color::Message(0);
const ConsoleWriter::Color ConsoleWriter::Color::Reset  (0);

const ConsoleWriter::Color ConsoleWriter::Color::Red          (FOREGROUND_RED   );
const ConsoleWriter::Color ConsoleWriter::Color::Green        (FOREGROUND_GREEN );
const ConsoleWriter::Color ConsoleWriter::Color::Blue         (FOREGROUND_BLUE  );
const ConsoleWriter::Color ConsoleWriter::Color::Yellow       (FOREGROUND_RED   | FOREGROUND_GREEN     );
const ConsoleWriter::Color ConsoleWriter::Color::Cyan         (FOREGROUND_GREEN | FOREGROUND_BLUE      );
const ConsoleWriter::Color ConsoleWriter::Color::Magenta      (FOREGROUND_RED   | FOREGROUND_BLUE      );
const ConsoleWriter::Color ConsoleWriter::Color::White        (FOREGROUND_RED   | FOREGROUND_GREEN     | FOREGROUND_BLUE      );

const ConsoleWriter::Color ConsoleWriter::Color::RedBright    (FOREGROUND_RED   | FOREGROUND_INTENSITY );
const ConsoleWriter::Color ConsoleWriter::Color::GreenBright  (FOREGROUND_GREEN | FOREGROUND_INTENSITY );
const ConsoleWriter::Color ConsoleWriter::Color::BlueBright   (FOREGROUND_BLUE  | FOREGROUND_INTENSITY );
const ConsoleWriter::Color ConsoleWriter::Color::YellowBright (FOREGROUND_RED   | FOREGROUND_GREEN     | FOREGROUND_INTENSITY );
const ConsoleWriter::Color ConsoleWriter::Color::CyanBright   (FOREGROUND_GREEN | FOREGROUND_BLUE      | FOREGROUND_INTENSITY );
const ConsoleWriter::Color ConsoleWriter::Color::MagentaBright(FOREGROUND_RED   | FOREGROUND_BLUE      | FOREGROUND_INTENSITY );
const ConsoleWriter::Color ConsoleWriter::Color::WhiteBright  (FOREGROUND_RED   | FOREGROUND_GREEN     | FOREGROUND_BLUE      | FOREGROUND_INTENSITY);

#else
#include <cstdio>
#include <cstdlib>
const ConsoleWriter::Color ConsoleWriter::Color::Error  (31);
const ConsoleWriter::Color ConsoleWriter::Color::Warning(33);
const ConsoleWriter::Color ConsoleWriter::Color::Notify (34); // was 134
const ConsoleWriter::Color ConsoleWriter::Color::Notify2(30); // was 137
const ConsoleWriter::Color ConsoleWriter::Color::NotifyH(35); // was 135
const ConsoleWriter::Color ConsoleWriter::Color::Action (36); // was 136
const ConsoleWriter::Color ConsoleWriter::Color::Proper (32); // was 132
const ConsoleWriter::Color ConsoleWriter::Color::Message(0); // was 0
const ConsoleWriter::Color ConsoleWriter::Color::Reset  (0);

const ConsoleWriter::Color ConsoleWriter::Color::Red          (31);
const ConsoleWriter::Color ConsoleWriter::Color::Green        (32);
const ConsoleWriter::Color ConsoleWriter::Color::Blue         (34);
const ConsoleWriter::Color ConsoleWriter::Color::Yellow       (33);
const ConsoleWriter::Color ConsoleWriter::Color::Cyan         (36);
const ConsoleWriter::Color ConsoleWriter::Color::Magenta      (35);
const ConsoleWriter::Color ConsoleWriter::Color::White        (37);

const ConsoleWriter::Color ConsoleWriter::Color::RedBright    (131);
const ConsoleWriter::Color ConsoleWriter::Color::GreenBright  (132);
const ConsoleWriter::Color ConsoleWriter::Color::BlueBright   (134);
const ConsoleWriter::Color ConsoleWriter::Color::YellowBright (133);
const ConsoleWriter::Color ConsoleWriter::Color::CyanBright   (136);
const ConsoleWriter::Color ConsoleWriter::Color::MagentaBright(135);
const ConsoleWriter::Color ConsoleWriter::Color::WhiteBright  (137);
#endif

ConsoleWriter::ConsoleWriter(std::ostream& out):
	FileWriter(),
	out(out),
	ignoreColors(false) {
		
        if(&out==&std::cout) {
		kindOfStream = 1;
	} else if(&out==&std::cerr) {
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
	if(ignoreColors) { return * this; }
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
