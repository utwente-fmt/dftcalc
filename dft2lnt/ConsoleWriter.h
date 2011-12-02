#ifndef CONSOLEWRITER_H
#define CONSOLEWRITER_H

#include "FileWriter.h"

class ConsoleWriter: public FileWriter {
public:
	class Color {
	private:
		int color;
	public:
		
		static const ConsoleWriter::Color Error;
		static const ConsoleWriter::Color Warning;
		static const ConsoleWriter::Color Notify;
		static const ConsoleWriter::Color Action;
		static const ConsoleWriter::Color Proper;
		static const ConsoleWriter::Color Message;
		static const ConsoleWriter::Color Reset;
		
		Color(int color): color(color) {
		}
		int getColorCode() const { return color; }
	};

	std::ostream& out;

	int kindOfStream;

	std::stack<Color> colorStack;
public:

	ConsoleWriter(std::ostream& out);

	virtual ostream& ss() {
		if(sss.size()==1) return out;
		return *sss.top();
	}

	virtual ConsoleWriter& operator<<(ConsoleWriter::Color color);

	virtual ConsoleWriter& appendLine(const string& s) {
		appendPrefix();
		append(s);
		appendPostfix();
		return *this;
	}

	virtual ConsoleWriter& append(const string& s) {
		ss() << s;
		return *this;
	}

	virtual ConsoleWriter& append(int i) {
		ss() << i;
		return *this;
	}

	virtual ConsoleWriter& append(unsigned int i) {
		ss() << i;
		return *this;
	}

	virtual ConsoleWriter& operator<<(const string& s) {
		ss() << s;
		return *this;
	}

	virtual ConsoleWriter& operator<<(int i) {
		ss() << i;
		return *this;
	}

	virtual ConsoleWriter& operator<<(unsigned int i) {
		ss() << i;
		return *this;
	}

	virtual ConsoleWriter& operator<<(long int i) {
		ss() << i;
		return *this;
	}

	virtual ConsoleWriter& operator<<(long unsigned int i) {
		ss() << i;
		return *this;
	}

	virtual ConsoleWriter& operator<<(const FileWriterOption& option) {
		FileWriter::operator<<(option);
		return *this;
	}

	virtual ConsoleWriter& appendPrefix() {
		append(applyprefix);
		return *this;
	}

	virtual ConsoleWriter& appendPostfix() {
		append(postfix);
		return *this;
	}
};

#endif //CONSOLEWRITER_H


