#ifndef FILEWRITER_H
#define FILEWRITER_H

#include <sstream>

/**
 * The StringWriter class acts as a buffer to write to. It uses a stringstream
 * to write to. The added feature is automatic indentation.
 */
class FileWriter {
private:
	int indentation;
	string prefix;
	string postfix;
public:
	stringstream ss;
	string applyprefix;
	string applypostfix;

	FileWriter(): indentation(0), prefix("\t"), postfix("\n"), applyprefix(""), applypostfix(postfix) {
	}

	/**
	 * Add the specified string to the stream. The string will be prefixed
	 * based on the currently set prefix and indentation level. The currently
	 * set postfix will be appended after the string.
	 * @param s The String to add.
	 */
	FileWriter& appendLine(const string& s) {
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
	FileWriter& append(const string& s) {
		ss << s;
		return *this;
	}

	/**
	 * Add the specified string to the stream.
	 * Nothing will be prefixed or postfixed.
	 * @param s The String to add.
	 */
	FileWriter& operator<<(const string& s) {
		ss << s;
		return *this;
	}

	/**
	 * Add the specified integer to the stream converted to a string.
	 * Nothing will be prefixed or postfixed.
	 * @param s The String to add.
	 */
	FileWriter& append(int i) {
		ss << i;
		return *this;
	}

	/**
	 * Add the specified integer to the stream converted to a string.
	 * Nothing will be prefixed or postfixed.
	 * @param s The String to add.
	 */
	FileWriter& append(unsigned int i) {
		ss << i;
		return *this;
	}

	/**
	 * Add the specified integer to the stream converted to a string.
	 * Nothing will be prefixed or postfixed.
	 * @param i The interger to add.
	 */
	FileWriter& operator<<(int i) {
		ss << i;
		return *this;
	}

	/**
	 * Add the specified integer to the stream converted to a string.
	 * Nothing will be prefixed or postfixed.
	 * @param i The interger to add.
	 */
	FileWriter& operator<<(unsigned int i) {
		ss << i;
		return *this;
	}

	/**
	 * Add the specified integer to the stream converted to a string.
	 * Nothing will be prefixed or postfixed.
	 * @param i The interger to add.
	 */
	FileWriter& operator<<(long int i) {
		ss << i;
		return *this;
	}

	/**
	 * Add the specified integer to the stream converted to a string.
	 * Nothing will be prefixed or postfixed.
	 * @param i The interger to add.
	 */
	FileWriter& operator<<(long unsigned int i) {
		ss << i;
		return *this;
	}

	/**
	 * Append the prefix based on the currently set prefix and current
	 * indentation level.
	 */
	FileWriter& appendPrefix() {
		append(applyprefix);
		return *this;
	}

	/**
	 * Append the postfix based on the currently set prefix and current
	 * indentation level.
	 */
	FileWriter& appendPostfix() {
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
	 * Returns the current contents of the buffer.
	 * @return The current contents of the buffer.
	 */
	string toString() {
		return ss.str();
	}

};



#endif // FILEWRITER_H