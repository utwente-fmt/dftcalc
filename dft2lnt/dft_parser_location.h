/*
 * dft_parser_location.cpp
 * 
 * Part of dft2lnt library - a library containing read/write operations for DFT
 * files in Galileo format and translating DFT specifications into Lotos NT.
 * 
 * @author Freark van der Berg
 */

#ifndef DFT_PARSER_LOCATION_H
#define DFT_PARSER_LOCATION_H

#include <iostream>
#include <sstream>

class Location {
private:
	std::string filename;
	int first_line;
	int first_column;
	int last_line;
	int last_column;
public:
	const std::string& getFileName() const {
		return filename;
	}
	void setFileName(const std::string& filename) {
		this->filename = filename;
	}

	int getFirstLine() const {
		return first_line;
	}
	int getFirstColumn() const {
		return first_column;
	}
	int getLastLine() const {
		return last_line;
	}
	int getLastColumn() const {
		return last_column-1;
	}
	
	bool isNull() const {
		return first_line==0
		    && last_line==0
			&& filename=="";
	}
	
	void nullify() {
		first_line   = last_line  = 0;
		first_column = last_column = 1;
		filename = "";
	}
	
	void reset() {
		first_column = first_line = 1;
		last_column  = last_line  = 1;
	}
	
	Location():
		filename(""),
		first_line(0),
		first_column(1),
		last_line(0),
		last_column(1) {
	}
	Location(std::string filename):
		filename(filename),
		first_line(0),
		first_column(1),
		last_line(0),
		last_column(1) {
	}
	Location(std::string filename, int first_line):
		filename(filename),
		first_line(first_line),
		first_column(1),
		last_line(first_line),
		last_column(1) {
	}
	Location(std::string filename, int first_line, int last_line):
		filename(filename),
		first_line(first_line),
		first_column(1),
		last_line(last_line),
		last_column(1) {
	}
	Location(std::string filename, int first_line, int first_column, int last_line, int last_column):
		filename(filename),
		first_line(first_line),
		first_column(first_column),
		last_line(last_line),
		last_column(last_column) {
	}
	std::string toString() const {
		std::stringstream ss;
		print(ss);
		return ss.str();
	}
	
	Location upTo(const Location& locOther) {
		if(isNull()) {
			return locOther;
		}
		if(locOther.isNull()) {
			return *this;
		}
		Location l;
		l.filename     = filename;
		l.first_line   = first_line;
		l.first_column = first_column;
		l.last_line    = locOther.first_line;
		l.last_column  = locOther.first_column;
		return l;
	}
	
	Location upToAndIncluding(const Location& locOther) {
		if(isNull()) {
			return locOther;
		}
		if(locOther.isNull()) {
			return *this;
		}
		Location l;
		l.filename     = filename;
		l.first_line   = first_line;
		l.first_column = first_column;
		l.last_line    = locOther.last_line;
		l.last_column  = locOther.last_column;
		return l;
	}
	
	Location afterUpTo(const Location& locOther) {
		if(isNull()) {
			return locOther;
		}
		if(locOther.isNull()) {
			return *this;
		}
		Location l;
		l.filename     = filename;
		l.first_line   = last_line;
		l.first_column = last_column;
		l.last_line    = locOther.first_line;
		l.last_column  = locOther.first_column;
		return l;
	}

	Location afterUpToAndIncluding(const Location& locOther) {
		Location l;
		l.filename     = filename;
		l.first_line   = last_line;
		l.first_column = last_column;
		l.last_line    = locOther.last_line;
		l.last_column  = locOther.last_column;
		return l;
	}
	
	void set(const std::string& filename, int line) {
		first_column   = 1;
		first_line     = line;
		last_column    = 1;
		last_line      = line;
		this->filename = filename;
	}
	
	void setToLastOf(const Location& locOther) {
		first_line   = last_line   = locOther.last_line;
		first_column = last_column = locOther.last_column;
		filename     = locOther.filename;
		//filename     = currentParser->getCurrentFileName();
	}
	void println(std::ostream& ss) const {
		print(ss);
		ss << std::endl;
	}
	void print(std::ostream& ss) const {
		ss << filename;
		if(getFirstLine() > 0) {
			ss << ":";
			if( getFirstLine() >= getLastLine()) {
				if( getFirstColumn() >= getLastColumn()) {
					if(getFirstColumn() > 0) {
						ss << getFirstLine();
						ss << ".";
						ss << getFirstColumn();
					} else {
						ss << getFirstLine();
					}
				} else {
					ss << getFirstLine();
					ss << ".";
					ss << getFirstColumn();
					ss << "-";
					ss << getLastColumn();
				}
			} else {
				if(getFirstColumn() > 0) {
					ss << getFirstLine();
					ss << ".";
					ss << getFirstColumn();
					ss << "-";
					ss << getLastLine();
					ss << ".";
					ss << getLastColumn();
				} else {
					ss << getFirstLine();
					ss << "-";
					ss << getLastLine();
				}
			}
		}
	}
	
	void step() {
		first_column = last_column;
		first_line   = last_line;
	}
	
	void advanceCharacters(int n) {
		last_column += n;
	}
	
	void advanceLines(int n) {
		last_column = 1;
		last_line  += n;
	}
	
	Location end() {
		Location l;
		l.filename = filename;
		l.first_line = last_line;
		l.first_column = last_column;
		l.last_line = last_line;
		l.last_column = last_column;
		return l;
	}

	Location begin() {
		Location l;
		l.filename = filename;
		l.first_line = first_line;
		l.first_column = first_column;
		l.last_line = first_line;
		l.last_column = first_column;
		return l;
	}
	
};

#define YYLTYPE_IS_DECLARED 1

#define YYLLOC_DEFAULT(This, Other, N) do { \
	if (N)                                                        \
	{                                                             \
		(This) = (YYRHSLOC (Other, 1)).upToAndIncluding(YYRHSLOC (Other, N)); \
	}                                                             \
	else                                                          \
	{ /* empty Other */                                           \
		(This).setToLastOf(YYRHSLOC (Other, 0));                  \
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
  (Loc).first_column = (Loc).last_column = (Loc).last_column;     \
  (Loc).first_line = (Loc).last_line;

/* Restart: move the first cursor to the last position. */
# define LOCATION_INC(Loc)                   \
  (Loc).first_column = (Loc).last_column = (Loc).last_column + 1;     \
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
