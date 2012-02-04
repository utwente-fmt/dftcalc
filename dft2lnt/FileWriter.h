/*
 * FileWriter.h
 * 
 * Part of a general library.
 * 
 * @author Freark van der Berg
 */

#ifndef FILEWRITER_H
#define FILEWRITER_H

#include <sstream>
#include <stack>
#include <string>
#include <assert.h>

using namespace std;

/**
 * The StringWriter class acts as a buffer to write to. It uses a stringstream
 * to write to. The added feature is automatic indentation.
 * It uses a stack of stringstream objects. Stream and append operations are
 * performed on the topmost stream. This can be used to for example outline
 * a string comprised of multiple append operations, by doing push, outline,
 * append, append, ..., pop.
 */
class FileWriter {
protected:

//	class WidthAdjuster {
//	public:
//		unsigned int width;
//		unsigned int left;
//		char fill;
//	};

	int indentation;
	string prefix;
	string postfix;
	std::stack<stringstream*> sss;
	
public:

	/**
	 * Streamable option class. When streamed, can change some setting of the
	 * FileWriter object.
	 */
	class FileWriterOption {
	public:
		enum Option {
			POP,
			PUSH
		};
		Option option;
		FileWriterOption(Option option): option(option) {
		}
	};

	static const FileWriterOption _pop;
	static const FileWriterOption _push;

	string applyprefix;
	string applypostfix;

	/**
	 * Creates a new FileWriter object with the specified settings.
	 * @param indentation Start with this level of indentation. Default is 0.
	 * @param prefix The prefix to prepend <indentlevel> times to every line. Default is "\t"
	 * @param postfix The postfix to append a single time to every line. Default is "\n"
	 */
	FileWriter(int indentation = 0, std::string prefix = "\t", std::string postfix = "\n"): indentation(indentation), prefix(prefix), postfix(postfix), applyprefix(""), applypostfix(postfix) {
		sss.push(new stringstream());
	}
	
	virtual ~FileWriter() {
		while(sss.size()>0) {
			delete sss.top();
			sss.pop();
		}
	}
	
	/**
	 * Pushed a new stringstream object on the stream stack.
	 * Following stream and append operations will be performed on
	 * the new stringstream object.
	 */
	void push() {
		sss.push(new stringstream());
	}
	
	/**
	 * Appends the contents of the topmost stringstream to the second
	 * stringstream and pops the topmost stringstream from the stream stack.
	 * Following stream and append operations will be performed on
	 * the now topmost stringstream object.
	 * If there is only one stringstream object on the stack, nothing is done.
	 */
	void pop() {
		if(sss.size()>1) {
			std::string s;
			s = sss.top()->str();
			delete sss.top();
			sss.pop();
			ss() << s;
		}
	}
	
	/**
	 * Returns the topmost stringstream object.
	 * @return The topmost stringstream object.
	 */
	virtual ostream& ss() {
		return *sss.top();
	}

	/**
	 * Returns the topmost stringstream object.
	 * @return The topmost stringstream object.
	 */
	stringstream& getStringStream() {
		return *sss.top();
	}
	
	/**
	 * Add the specified string to the stream. The string will be prefixed
	 * based on the currently set prefix and indentation level. The currently
	 * set postfix will be appended after the string.
	 * @param s The String to add.
	 */
	virtual FileWriter& appendLine(const string& s) {
		appendPrefix();
		append(s);
		appendPostfix();
		return *this;
	}

	/**
	 * Add the specified string to the stream.
	 * Nothing will be prefixed or postfixed.
	 * @param s The String to add.
	 */
	virtual FileWriter& append(const string& s) {
		ss() << s;
		return *this;
	}

	/**
	 * Add the specified integer to the stream converted to a string.
	 * Nothing will be prefixed or postfixed.
	 * @param s The String to add.
	 */
	virtual FileWriter& append(int i) {
		ss() << i;
		return *this;
	}

	/**
	 * Add the specified integer to the stream converted to a string.
	 * Nothing will be prefixed or postfixed.
	 * @param s The String to add.
	 */
	virtual FileWriter& append(unsigned int i) {
		ss() << i;
		return *this;
	}

	/**
	 * Add the specified string to the stream.
	 * Nothing will be prefixed or postfixed.
	 * @param s The String to add.
	 */
	virtual FileWriter& operator<<(const string& s) {
		ss() << s;
		return *this;
	}

	/**
	 * Add the specified integer to the stream converted to a string.
	 * Nothing will be prefixed or postfixed.
	 * @param i The integer to add.
	 */
	virtual FileWriter& operator<<(int i) {
		ss() << i;
		return *this;
	}

	/**
	 * Add the specified integer to the stream converted to a string.
	 * Nothing will be prefixed or postfixed.
	 * @param i The integer to add.
	 */
	virtual FileWriter& operator<<(unsigned int i) {
		ss() << i;
		return *this;
	}

	/**
	 * Add the specified integer to the stream converted to a string.
	 * Nothing will be prefixed or postfixed.
	 * @param i The integer to add.
	 */
	virtual FileWriter& operator<<(long int i) {
		ss() << i;
		return *this;
	}

	/**
	 * Add the specified integer to the stream converted to a string.
	 * Nothing will be prefixed or postfixed.
	 * @param i The integer to add.
	 */
	virtual FileWriter& operator<<(long unsigned int i) {
		ss() << i;
		return *this;
	}

	/**
	 * Add the specified float to the stream converted to a string.
	 * Nothing will be prefixed or postfixed.
	 * @param f The float to add.
	 */
	virtual FileWriter& operator<<(float f) {
		ss() << f;
		return *this;
	}

	/**
	 * Add the specified double to the stream converted to a string.
	 * Nothing will be prefixed or postfixed.
	 * @param d The double to add.
	 */
	virtual FileWriter& operator<<(double d) {
		ss() << d;
		return *this;
	}

	/**
	 * Add the specified long double to the stream converted to a string.
	 * Nothing will be prefixed or postfixed.
	 * @param ld The double to add.
	 */
	virtual FileWriter& operator<<(long double ld) {
		ss() << ld;
		return *this;
	}

	/**
	 * Stream the specified FileWriterOption.
	 * Nothing will be prefixed or postfixed.
	 * @param option The FileWriterOption to stream.
	 */
	virtual FileWriter& operator<<(const FileWriterOption& option) {
		switch(option.option) {
			case FileWriterOption::POP:
				pop();
				break;
			case FileWriterOption::PUSH:
				push();
				break;
		}
		return *this;
	}
	
	/**
	 * Stream the contents of the other FileWriter to this one.
	 * Nothing will be prefixed or postfixed.
	 * @param other The FileWriter to stream the contents of.
	 */
	virtual FileWriter& operator<<(const FileWriter& other) {
		ss() << other.toString();
		return *this;
	}

//	FileWriter& operator<<(std::ios_base& base) {
//		ss() << base;
//		return *this;
//	}

	/**
	 * Append the prefix based on the currently set prefix and current
	 * indentation level.
	 */
	virtual FileWriter& appendPrefix() {
		append(applyprefix);
		return *this;
	}

	/**
	 * Append the postfix based on the currently set prefix and current
	 * indentation level.
	 */
	virtual FileWriter& appendPostfix() {
		append(postfix);
		return *this;
	}

	/**
	 * Increase the indentation. Subsequently affected method calls will be
	 * prefixed with an additional prefix.
	 */
	void indent() {
		++indentation;
		applyprefix += prefix;
	}

	/**
	 * Decrease the indentation. Subsequently affected method calls will be
	 * prefixed with one less prefix. It is an error to call this if the
	 * indentation is already 0.
	 */
	void outdent() {
		assert(indentation>0);
		if(indentation>0) {
			--indentation;
			applyprefix = "";
			for(int i=indentation;i--;) {
				applyprefix += prefix;
			}
		}
	}

	/**
	 * The next object to be streamed will be outlined to the left, accoring
	 * to the specified settings.
	 * @param width The total number of character on which to outline.
	 * @param fill The character to use for filled up characters.
	 * For example outlineLeftNext(5,'0'); and then append(33) would result in
	 * the same as append("33000");
	 */
	void outlineLeftNext(unsigned int width, char fill) {
		ss().fill(fill);
		ss().width(width);
		ss() << left;
	}

	/**
	 * The next object to be streamed will be outlined to the right, accoring
	 * to the specified settings.
	 * @param width The total number of character on which to outline.
	 * @param fill The character to use for filled up characters.
	 * For example outlineRightNext(5,'0'); and then append(33) would result in
	 * the same as append("00033");
	 */
	void outlineRightNext(unsigned int width, char fill) {
		ss().fill(fill);
		ss().width(width);
		ss() << right;
	}

	/**
	 * Returns the current contents of the buffer.
	 * @return The current contents of the buffer.
	 */
	string toString() const {
		return sss.top()->str();
	}
	
	/**
	 * Clears the topmost stringstream, setting it to "".
	 */
	void clear() {
		sss.top()->str("");
	}

	/**
	 * Clears all the stringstream object in the stream stack,
	 * setting all to "".
	 */
	void clearAll() {
		while(sss.size()>1) {
			delete sss.top();
			sss.pop();
		}
		clear();
	}
};



#endif // FILEWRITER_H
