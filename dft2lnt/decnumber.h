#include <string>
#include <cstring>
#include <limits>
#include <cctype>
#include <cmath>
#include <exception>

template <class BT = unsigned int, class ET = long>
class decnumber
{
	/* Floating point arbitrary-precision number.
	 * Representation: s*i*10^e where s is the sign, and i and e are
	 * integers.
	 * e is stored as a standard integer (I doubt we'll need numbers
	 * bigger than 10^2^32 any time soon).
	 * i is stored as a base-N sequence of digits, where N is the
	 * largest multiple of 10 we can fit in a block (the sequence is
	 * stored big-endian).
	 */
private:
	static constexpr BT base = (std::numeric_limits<BT>::max() / 10) * 10;
	std::size_t num_blocks;
	/* Blocks store the integer portion of the number in base 250
	 * (largest number that fits but is conveniently 
	 * */
	BT *blocks;
	ET exponent;
	int sign;

	void mul_unnorm(BT val) {
		BT carry = 0;
		BT premul_max = base / val;
		for (std::size_t i = num_blocks - 1; i != SIZE_MAX; i--) {
			BT new_carry;
			new_carry = blocks[i] / premul_max;
			blocks[i] = (blocks[i] % premul_max) * val;
			if (base - blocks[i] < carry) {
				new_carry++;
				blocks[i] -= base;
			}
			blocks[i] += carry;
			carry = new_carry;
		}
		if (carry) {
			if (num_blocks == SIZE_MAX)
				throw std::overflow_error("Number way too big.");
			BT *tmp = new BT[num_blocks + 1];
			if (blocks != nullptr) {
				memcpy(tmp+1, blocks, num_blocks * sizeof(BT));
				delete[] blocks;
			}
			num_blocks++;
			blocks = tmp;
			blocks[0] = carry;
		}
	}

	void normalize() {
		while (blocks && !(blocks[num_blocks - 1] % 10)) {
			divint(10);
			exponent++;
		}
		if (!blocks)
			exponent = 0;
	}

	void parse_string(std::string num) {
		std::size_t pos = 0;
		bool past_point = false;
		while (isspace(num[pos]))
			pos++;
		if (num[pos] == '-') {
			sign = -1;
			pos++;
		} else {
			sign = 1;
			if (num[pos] == '+')
				pos++;
		}
		while (num[pos] == '0')
			pos++;
		num_blocks = 0;
		exponent = 0;
		blocks = nullptr;

		while (num[pos] != 0 && num[pos] != 'e' && num[pos] != 'E') {
			if (num[pos] == '.') {
				if (past_point) {
					delete[] blocks;
					throw std::runtime_error("Multiple decimal points");
				}
				past_point = true;
				pos++;
				continue;
			}
			if (!isdigit(num[pos])) {
				delete[] blocks;
				throw std::runtime_error("Unexpected character in number: " + num);
			}
			mul_unnorm(10);
			if (num_blocks == 0) {
				blocks = new BT[1];
				blocks[0] = 0;
				num_blocks = 1;
			}
			blocks[num_blocks - 1] += num[pos] - '0';
			if (past_point) {
				if (exponent == std::numeric_limits<ET>::min())
				{
					delete[] blocks;
					throw std::overflow_error("Exponent out of range: " + num);
				}
				exponent--;
			}
			pos++;
		}
		if (!num[pos]) {
			normalize();
			return;
		}
		/* Exponent remains */
		pos++; /* Skip 'e' */
		ET additional_exponent = 0;
		ET emax = std::numeric_limits<ET>::max();
		int exp_sign = 1;
		if (num[pos] == '-') {
			exp_sign = -1;
			pos++;
		} else if (num[pos] == '+') {
			pos++;
		}
		while (num[pos]) {
			if (!isdigit(num[pos])) {
				delete[] blocks;
				throw std::runtime_error("Unexpected character in number: " + num);
			}
			if (additional_exponent < ((emax - 9) / 10) - 9) {
				additional_exponent *= 10;
				additional_exponent += num[pos] - '0';
			} else {
				delete[] blocks;
				throw std::overflow_error("Exponent out of range: " + num);
			}
			pos++;
		}
		additional_exponent *= exp_sign;
		if (exponent <= 0 && sign == 1) {
			exponent += additional_exponent;
		} else if (exponent >= 0 && sign == -1) {
			exponent -= additional_exponent;
		} else if (sign == 1) {
			if (exponent > emax - additional_exponent) {
				delete[] blocks;
				throw std::overflow_error("Exponent out of range: " + num);
			}
			exponent += additional_exponent;
		} else { /* sign == -1 */
			ET emin = std::numeric_limits<ET>::min();
			if (exponent < emin + additional_exponent) {
				delete[] blocks;
				throw std::overflow_error("Exponent out of range: " + num);
			}
			exponent -= additional_exponent;
		}
		normalize();
	}
public:
	decnumber() = delete;

	decnumber(const decnumber<BT, ET> &other) {
		num_blocks = other.num_blocks;
		if (num_blocks) {
			blocks = new BT[num_blocks];
			memcpy(blocks, other.blocks, num_blocks * sizeof(*blocks));
		} else {
			blocks = nullptr;
		}
		exponent = other.exponent;
		sign = other.sign;
	}

	decnumber(decnumber<BT, ET> &&other)
		: num_blocks(other.num_blocks),
		  blocks(other.blocks),
		  exponent(other.exponent),
		  sign(other.sign)
	{
		other.blocks = nullptr;
	}

	decnumber(std::string num) {
		parse_string(num);
	}

	decnumber(long double v) {
		long double frac, ipart;
		frac = modf(v, &ipart);
		if (frac) {
			/* TODO: exact conversion. */
			parse_string(std::to_string(v));
			return;
		}
		blocks = nullptr;
		num_blocks = 0;
		sign = 1;
		exponent = 0;
		if (v < 0) {
			sign = -1;
			v = -v;
		}
		while (ipart) {
			mul_unnorm(2);
			if (num_blocks == 0) {
				blocks = new BT[1];
				blocks[0] = 0;
				num_blocks = 1;
			}
			frac = modf(ipart / 2, &ipart);
			if (frac)
				blocks[num_blocks - 1]++;
		}
		normalize();
	}

	~decnumber() {
		delete[] blocks;
	}

	decnumber<BT, ET>& operator=(const decnumber<BT, ET> &other) {
		if (this != &other) {
			delete[] blocks;
			num_blocks = other.num_blocks;
			if (num_blocks) {
				blocks = new BT[num_blocks];
				memcpy(blocks, other.blocks, num_blocks * sizeof(BT));
			} else {
				blocks = nullptr;
			}
			exponent = other.exponent;
			sign = other.sign;
		}
		return *this;
	}

	decnumber<BT, ET>& operator=(decnumber<BT, ET> &&other) {
		if (this != &other) {
			delete[] blocks;
			blocks = other.blocks;
			other.blocks = nullptr;
			num_blocks = other.num_blocks;
			exponent = other.exponent;
			sign = other.sign;
		}
		return *this;
	}

	decnumber<BT, ET> operator+(const decnumber<BT, ET> &other) const {
		if (other.num_blocks == 0)
			return *this;
		if (num_blocks == 0)
			return other;
		decnumber<BT, ET> ret(*this);
		ret += other;
		return ret;
	}

	decnumber<BT, ET> operator+=(const decnumber<BT, ET> &other) {
		if (other.num_blocks == 0)
			return *this;
		if (num_blocks == 0) {
			*this = other;
			return *this;
		}
		if (other.sign == -1) {
			decnumber<BT, ET> ret(other);
			ret.sign *= -1;
			*this = *this - ret;
			return *this;
		}

		if (sign == -1) {
			sign = 1;
			*this = other - *this;
			sign *= -1;
			return *this;
		}
		decnumber<BT, ET> tmp(0);
		const decnumber<BT, ET> *add;
		if (other.exponent >= exponent) {
			add = &other;
			while (add->exponent > exponent) {
				mul_unnorm(10);
				exponent--;
			}
		} else {
			tmp = other;
			while (exponent > tmp.exponent) {
				tmp.mul_unnorm(10);
				tmp.exponent--;
			}
			add = &tmp;
		}
		BT carry = 0;
		for (std::size_t i = num_blocks - 1; i != SIZE_MAX; i--) {
			if (blocks[i] == (base - 1) && carry) {
				blocks[i] = 0;
				carry = 1;
			} else {
				blocks[i] += carry;
				carry = 0;
			}
			if (blocks[i] > base - add->blocks[i]) {
				carry = 1;
				blocks[i] += base - add->blocks[i];
			} else {
				carry = 0;
				blocks[i] += add->blocks[i];
			}
		}
		if (carry) {
			if (num_blocks == SIZE_MAX);
				throw std::overflow_error("Addition result too large");
			BT *bl;
			bl = new BT[num_blocks + 1];
			bl[0] = carry;
			memcpy(bl+1, blocks, num_blocks * sizeof(*bl));
			num_blocks++;
			delete[] blocks;
			blocks = bl;
		}
		normalize();
		return *this;
	}

	decnumber<BT, ET> operator-(const decnumber<BT, ET> &other) const {
		if (other.num_blocks == 0)
			return *this;
		if (num_blocks == 0) {
			decnumber<BT, ET> ret(other);
			ret.sign *= -1;
			return ret;
		}
		if (other.sign == -1) {
			decnumber<BT, ET> ret(other);
			ret.sign *= -1;
			return *this + ret;
		}

		decnumber<BT, ET> ret(*this);
		if (sign == -1) {
			ret.sign = 1;
			ret = ret + other;
			ret.sign *= -1;
			return ret;
		}
		decnumber<BT, ET> tmp(0);
		const decnumber<BT, ET> *sub;
		if (other.exponent <= exponent) {
			sub = &other;
			while (sub->exponent < ret.exponent) {
				ret.mul_unnorm(10);
				ret.exponent--;
			}
		} else {
			tmp = other;
			while (ret.exponent < tmp.exponent) {
				tmp.mul_unnorm(10);
				tmp.exponent--;
			}
			sub = &tmp;
		}
		BT carry = 0;
		for (std::size_t i = ret.num_blocks - 1; i != SIZE_MAX; i--) {
			if (ret.blocks[i] < carry) {
				ret.blocks[i] += base + carry;
				carry = 1;
			} else {
				ret.blocks[i] -= carry;
				carry = 0;
			}
			if (ret.blocks[i] < sub->blocks[i]) {
				carry += 1;
				ret.blocks[i] += base + sub->blocks[i];
			} else {
				ret.blocks[i] -= sub->blocks[i];
			}
		}
		if (carry) {
			return other - *this;
		} else {
			ret.normalize();
			return ret;
		}
	}

	decnumber<BT, ET>& operator*=(const decnumber<BT, ET> &other) {
		if (other.num_blocks == 0 || num_blocks == 0) {
			num_blocks = 0;
			exponent = 0;
			sign = 1;
			delete[] blocks;
			blocks = nullptr;
			return *this;
		}
		int target_sign = 1;
		if (sign != other.sign)
			target_sign = -1;

		ET emax = std::numeric_limits<ET>::max();
		ET emin = std::numeric_limits<ET>::max();
		ET target_exponent = other.exponent;
		if ((target_exponent > 0 && exponent > emax - target_exponent)
		 || (target_exponent < 0 && exponent < emin - target_exponent))
		{
			throw std::overflow_error("Multiplication result too large" + std::to_string(target_exponent) + " plus " + std::to_string(exponent));
		}
		target_exponent += exponent;

		decnumber<BT, ET> multiplier(0);
		multiplier.blocks = blocks;
		multiplier.exponent = exponent;
		multiplier.num_blocks = num_blocks;
		num_blocks = 0;
		blocks = nullptr;
		exponent = 0;
		sign = 1;

		decnumber<BT, ET> multiplicand(other);
		std::size_t i;
		for (i = multiplicand.num_blocks - 1; i != SIZE_MAX; i--) {
			decnumber<BT, ET> tmp(multiplier);
			tmp.mul_unnorm(multiplicand.blocks[i]);
			*this += tmp;
			multiplier.mul_unnorm(base);
		}
		sign = target_sign;
		exponent = target_exponent;
		normalize();
		return *this;
	}

	decnumber<BT, ET> operator*(const decnumber<BT, ET> &other) const {
		if (other.num_blocks == 0 || num_blocks == 0)
			return decnumber<BT, ET>(0);
		decnumber<BT, ET> ret(*this);
		ret *= other;
		return ret;
	}

	std::string str() const {
		if (!num_blocks)
			return "0";
		std::string ret;
		decnumber<BT, ET> num(*this);
		while (num.num_blocks) {
			BT digit = num.blocks[num.num_blocks - 1] % 10;
			ret = (char)('0' + digit) + ret;
			num.divint(10);
		}
		ret += std::string("e") + std::to_string(exponent);
		return ret;
	}

	explicit operator double() const {
		return std::stod(str());
	}

	bool is_zero() const {
		return num_blocks == 0;
	}

	void divint(BT num) {
		if (num < -base || num > base)
			throw std::invalid_argument("Division by numbers larger than the internal base is currently not supported.");
		if (base % num)
			throw std::invalid_argument("Divisor currently must be multiple of internal base.");
		BT carry = 0;
		if (num < 0) {
			sign = -sign;
			num = -num;
		}
		BT carry_mul = base / num;
		for (std::size_t i = 0; i < num_blocks; i++) {
			BT new_carry;
			new_carry = blocks[i] % num;
			blocks[i] /= num;
			blocks[i] += carry * carry_mul;
			carry = new_carry;
		}
		if (!num_blocks || blocks[0])
			return;
		num_blocks--;
		if (!num_blocks) {
			delete[] blocks;
			blocks = nullptr;
		} else {
			BT *new_blocks = new BT[num_blocks];
			memcpy(new_blocks, blocks + 1, num_blocks * sizeof(BT));
			delete[] blocks;
			blocks = new_blocks;
		}
	}
};
