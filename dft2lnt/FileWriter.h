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
	
	virtual ostream& ss() {
		return *sss.top();
	}

public:

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

	FileWriter(): indentation(0), prefix("\t"), postfix("\n"), applyprefix(""), applypostfix(postfix) {
		sss.push(new stringstream());
	}
	
	~FileWriter() {
		while(sss.size()>0) {
			delete sss.top();
			sss.pop();
		}
	}
	
	void push() {
		sss.push(new stringstream());
	}
	
	void pop() {
		std::string s;
		if(sss.size()>1) {
			s = sss.top()->str();
			delete sss.top();
			sss.pop();
		}
		ss() << s;
	}
	
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
	 * @param i The interger to add.
	 */
	virtual FileWriter& operator<<(int i) {
		ss() << i;
		return *this;
	}

	/**
	 * Add the specified integer to the stream converted to a string.
	 * Nothing will be prefixed or postfixed.
	 * @param i The interger to add.
	 */
	virtual FileWriter& operator<<(unsigned int i) {
		ss() << i;
		return *this;
	}

	/**
	 * Add the specified integer to the stream converted to a string.
	 * Nothing will be prefixed or postfixed.
	 * @param i The interger to add.
	 */
	virtual FileWriter& operator<<(long int i) {
		ss() << i;
		return *this;
	}

	/**
	 * Add the specified integer to the stream converted to a string.
	 * Nothing will be prefixed or postfixed.
	 * @param i The interger to add.
	 */
	virtual FileWriter& operator<<(long unsigned int i) {
		ss() << i;
		return *this;
	}

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

	void outlineLeftNext(unsigned int width, char fill) {
		ss().fill(fill);
		ss().width(width);
		ss() << left;
	}

	void outlineRightNext(unsigned int width, char fill) {
		ss().fill(fill);
		ss().width(width);
		ss() << right;
	}

	/**
	 * Returns the current contents of the buffer.
	 * @return The current contents of the buffer.
	 */
	string toString() {
		return sss.top()->str();
	}
	
	void clear() {
		sss.top()->str("");
	}

	void clearAll() {
		while(sss.size()>1) {
			delete sss.top();
			sss.pop();
		}
		clear();
	}
};



#endif // FILEWRITER_H
