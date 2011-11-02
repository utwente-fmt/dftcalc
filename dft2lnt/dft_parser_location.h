#ifndef DFT_PARSER_LOCATION_H
#define DFT_PARSER_LOCATION_H

#include <iostream>
#include <sstream>

typedef struct Location {
	std::string filename;
	int first_line;
	int first_column;
	int last_line;
	int last_column;
	Location():
		filename(""),
		first_line(0),
		first_column(0),
		last_line(0),
		last_column(0) {
	}
	Location(std::string filename):
		filename(filename),
		first_line(0),
		first_column(0),
		last_line(0),
		last_column(0) {
	}
	Location(std::string filename, int first_line):
		filename(filename),
		first_line(first_line),
		first_column(0),
		last_line(first_line),
		last_column(0) {
	}
	Location(std::string filename, int first_line, int last_line):
		filename(filename),
		first_line(first_line),
		first_column(0),
		last_line(last_line),
		last_column(0) {
	}
	Location(std::string filename, int first_line, int first_column, int last_line, int last_column):
		filename(filename),
		first_line(first_line),
		first_column(first_column),
		last_line(last_line),
		last_column(last_column) {
	}
	std::string toString() {
		std::stringstream ss;
		print(ss);
		return ss.str();
	}
	void print(std::ostream& ss) {
		ss << filename;
		if(first_line > 0) {
			ss << ":";
			if( first_line == last_line) {
				if(first_column > 0) {
					if( first_column == last_column) {
						ss << first_line;
						ss << ".";
						ss << first_column;
					} else {
						ss << first_line;
						ss << ".";
						ss << first_column;
						ss << "-";
						ss << last_column;
					}
				} else {
					ss << first_line;
				}
			} else {
				if(first_column > 0) {
					ss << first_line;
					ss << ".";
					ss << first_column;
					ss << "-";
					ss << last_line;
					ss << ".";
					ss << last_column;
				} else {
					ss << first_line;
					ss << "-";
					ss << last_line;
				}
			}
		}
	}
} Location;
#define YYLTYPE_IS_DECLARED 1

#define YYLLOC_DEFAULT(This, Other, N) do { \
	if (N)                                                        \
	{                                                             \
		(This).first_line   = YYRHSLOC (Other, 1).first_line;     \
		(This).first_column = YYRHSLOC (Other, 1).first_column;   \
		(This).last_line    = YYRHSLOC (Other, N).last_line;      \
		(This).last_column  = YYRHSLOC (Other, N).last_column;    \
		(This).filename     = YYRHSLOC (Other, 1).filename;       \
	}                                                             \
	else                                                          \
	{ /* empty Other */                                           \
		(This).first_line   = (This).last_line   = YYRHSLOC (Other, 0).last_line;   \
		(This).first_column = (This).last_column = YYRHSLOC (Other, 0).last_column; \
		(This).filename  = "";                                  \
	}                                                             \
} while(0)


typedef Location YYLTYPE;

/* Initialize LOC. */
# define LOCATION_RESET(Loc)                  \
  (Loc).first_column = (Loc).first_line = 1;  \
  (Loc).last_column =  (Loc).last_line = 1;

/* Advance of NUM lines. */
# define LOCATION_LINES(Loc, Num)             \
  (Loc).last_column = 1;                      \
  (Loc).last_line += Num;

/* Restart: move the first cursor to the last position. */
# define LOCATION_STEP(Loc)                   \
  (Loc).first_column = (Loc).last_column;     \
  (Loc).first_line = (Loc).last_line;

/* Set the first and last cursor to the beginning of the specified line
 * and set the file.
 */
# define LOCATION_SET(Loc,line,file)          \
  (Loc).first_column = 1;                     \
  (Loc).first_line = (line);                  \
  (Loc).last_column = 1;                      \
  (Loc).last_line = (line);                   \
  (Loc).filename = (file);

#define LOCATION_PRINT(Loc) \
  printf("Location: %s: %i.%i - %i.%i\n",(Loc).filename.c_str(),(Loc).first_line,(Loc).first_column,(Loc).last_line,(Loc).last_column)

#define compiler_parser_report_error(str) currentParser->getCC()->reportErrorAt(loc,str)
#define compiler_parser_report_warning(str) currentParser->getCC()->reportWarningAt(loc,str)
#define compiler_parser_report_errorAt(loc,str) { /*printf("@%i: ",__LINE__);*/ currentParser->getCC()->reportErrorAt(loc,str); }
#define compiler_parser_report_warningAt(loc,str) { currentParser->getCC()->reportWarningAt(loc,str); }


#endif // DFT_PARSER_LOCATION_H
