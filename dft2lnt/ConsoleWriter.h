/*
 * CompilerContext.h
 * 
 * Part of a general library.
 * 
 * @author Freark van der Berg
 */

#ifndef CONSOLEWRITER_H
#define CONSOLEWRITER_H

#include "FileWriter.h"

/**
 * The ConsoleWriter is a FileWriter with support to colour the output if the
 * specified output stream is std::cout or std::cerr.
 */
class ConsoleWriter: public FileWriter {
public:
	class Color {
	private:
		int color;
	public:
		
		static const ConsoleWriter::Color Error;
		static const ConsoleWriter::Color Warning;
		static const ConsoleWriter::Color Notify;
		static const ConsoleWriter::Color Notify2;
		static const ConsoleWriter::Color NotifyH;
		static const ConsoleWriter::Color Action;
		static const ConsoleWriter::Color Proper;
		static const ConsoleWriter::Color Message;
		static const ConsoleWriter::Color Reset;
		
		//static const ConsoleWriter::Color Black;
		static const ConsoleWriter::Color Red;
		static const ConsoleWriter::Color Green;
		static const ConsoleWriter::Color Blue;
		static const ConsoleWriter::Color Yellow;
		static const ConsoleWriter::Color Magenta;
		static const ConsoleWriter::Color Cyan;
		static const ConsoleWriter::Color White;
		//static const ConsoleWriter::Color BlackBright;
		static const ConsoleWriter::Color RedBright;
		static const ConsoleWriter::Color GreenBright;
		static const ConsoleWriter::Color BlueBright;
		static const ConsoleWriter::Color YellowBright;
		static const ConsoleWriter::Color MagentaBright;
		static const ConsoleWriter::Color CyanBright;
		static const ConsoleWriter::Color WhiteBright;

		Color(int color): color(color) {
		}
		Color(): color(0) {
		}
		int getColorCode() const { return color; }
	};
private:
	std::ostream& out;
	int kindOfStream;
	std::stack<Color> colorStack;
	bool ignoreColors;
public:

	/**
	 * Creates a new ConsoleWriter object.
	 * @param out The stream to output to.
	 */
	ConsoleWriter(std::ostream& out);

	/**
	 * Returns the topmost stream on the stack stream.
	 */
	virtual ostream& ss() {
		if(sss.size()==1) return out;
		return *sss.top();
	}

	/**
	 * Stream a Color object. The next object streamed will be of the specified
	 * colour. Often followed by push().
	 * @param color The Color the next streamed object will have.
	 * @return The Conwo
	 */
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

	virtual ConsoleWriter& operator<<(float f) {
		ss() << f;
		return *this;
	}

	virtual ConsoleWriter& operator<<(double d) {
		ss() << d;
		return *this;
	}

	virtual ConsoleWriter& operator<<(long double ld) {
		ss() << ld;
		return *this;
	}

	virtual ConsoleWriter& operator<<(const FileWriterOption& option) {
		FileWriter::operator<<(option);
		return *this;
	}

	virtual ConsoleWriter& operator<<(const FileWriter& other) {
		ss() << other.toString();
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
	
	virtual void setIgnoreColors(bool ignoreColors) {
		this->ignoreColors = ignoreColors;
	}
};

#endif //CONSOLEWRITER_H


