#include <string>
#include <cstring>
#include <limits>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <exception>
#include <stdexcept>

#ifndef DECNUMBER_H
#define DECNUMBER_H

#ifdef max
// Windows.h defines a max() macro that conflicts with C++ std::numeric_limits<T>::max
# undef max
# undef min
#endif
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
	BT *blocks;
	ET exponent;
	int sign;

	void mul_unnorm(BT val) {
		if (val == 0) {
			free(blocks);
			blocks = nullptr;
			num_blocks = 0;
			sign = 1;
			exponent = 0;
			return;
		}
		if (val < 0)
			throw std::invalid_argument("Unnormalized multiplication by negative numbers currently not supported.");
		if (val == 1)
			return;
		if (val == base) {
			if (num_blocks == SIZE_MAX)
				throw std::overflow_error("Number way too big.");
			BT *tmp = new BT[num_blocks + 1];
			memcpy(tmp, blocks, num_blocks * sizeof(BT));
			delete[] blocks;
			blocks = tmp;
			blocks[num_blocks] = 0;
			num_blocks++;
			return;
		}
		decnumber<BT, ET> old(*this);
		if (val > 2) {
			mul_unnorm(val / 2);
		}
		/* val == 2 */
		BT carry = 0;
		for (std::size_t i = num_blocks - 1; i != SIZE_MAX; i--) {
			BT new_carry = 0;
			/* This entirely depends on base being divisible by 2, which it
			 * is since we've chosen it to be a multiple of 10.
			 */
			if (blocks[i] >= base / 2) {
				blocks[i] -= base / 2;
				new_carry = 1;
			}
			blocks[i] *= 2;
			blocks[i] += carry;
			if (blocks[i] >= base) {
				new_carry++;
				blocks[i] -= base;
			}
			carry = new_carry;
		}
		if (carry) {
			if (num_blocks == SIZE_MAX)
				throw std::overflow_error("Number way too big.");
			BT *tmp = new BT[num_blocks + 1];
			memcpy(tmp+1, blocks, num_blocks * sizeof(BT));
			delete[] blocks;
			num_blocks++;
			blocks = tmp;
			blocks[0] = carry;
		}
		if (val & 1) {
			BT carry = 0;
			size_t i;
			for (i = 0; i < old.num_blocks; i++) {
				size_t pn = num_blocks - i - 1;
				size_t po = old.num_blocks - i - 1;
				if (blocks[pn] == base - carry) {
					blocks[pn] = 0;
				} else {
					blocks[pn] += carry;
					carry = 0;
				}
				if (blocks[pn] >= base - old.blocks[po]) {
					carry++;
					blocks[pn] -= base - old.blocks[po];
				} else {
					blocks[pn] += old.blocks[po];
				}
			}
			i = num_blocks - old.num_blocks;
			while (carry) {
				if (i) {
					i--;
					blocks[i]++;
					if (blocks[i] == base)
						blocks[i] = 0;
					else
						carry = 0;
				} else {
					if (num_blocks == SIZE_MAX)
						throw std::overflow_error("Number way too big.");
					BT *tmp = new BT[num_blocks + 1];
					memcpy(tmp+1, blocks, num_blocks * sizeof(BT));
					delete[] blocks;
					num_blocks++;
					blocks = tmp;
					blocks[0] = carry;
					carry = 0;
				}
			}
		}
	}

	void normalize() {
		while (blocks && !(blocks[num_blocks - 1] % 10)) {
			divint(10);
			exponent++;
		}
		while (num_blocks && blocks[0] == 0) {
			if (num_blocks == 0) {
				delete[] blocks;
				blocks = nullptr;
			} else {
				BT *tmp = new BT[num_blocks - 1];
				num_blocks--;
				memcpy(tmp, blocks + 1, num_blocks*sizeof(BT));
				delete[] blocks;
				blocks = tmp;
			}
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
				throw std::runtime_error("Unexpected character in number: '" + num + "'");
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
	//decnumber() :num_blocks(0), blocks(nullptr), sign(1), exponent(0)
	//{}
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

	decnumber(uintmax_t i) {
		blocks = nullptr;
		*this = i;
	}

	decnumber(intmax_t i) {
		blocks = nullptr;
		*this = i;
	}

	decnumber(int i) {
		blocks = nullptr;
		*this = (intmax_t)i;
	}

	decnumber(long double v) {
		blocks = nullptr;
		*this = v;
	}

	decnumber(double v) {
		blocks = nullptr;
		*this = (long double)v;
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

	decnumber<BT, ET>& operator=(uintmax_t i) {
		delete[] blocks;
		blocks = nullptr;
		num_blocks = 0;
		sign = 1;
		exponent = 0;
		uintmax_t rev = 0, bits = 0;
		if (i == 0)
			return *this;
		while (i) {
			rev <<= 1;
			if (i % 2)
				rev++;
			i /= 2;
			bits++;
		}
		while (bits--) {
			mul_unnorm(2);
			if (num_blocks == 0) {
				blocks = new BT[1];
				blocks[0] = 0;
				num_blocks = 1;
			}
			if (rev % 2)
				blocks[num_blocks - 1]++;
			rev /= 2;
		}
		normalize();
		return *this;
	}

	decnumber<BT, ET>& operator=(intmax_t i) {
		if (i >= 0) {
			*this = (uintmax_t)i;
		} else {
			*this = -(uintmax_t)i;
			sign = -1;
		}
		return *this;
	}

	decnumber<BT, ET>& operator=(long double v) {
		long double frac, ipart;
		delete[] blocks;
		blocks = nullptr;
		frac = modfl(v, &ipart);
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
			frac = modfl(ipart / 2, &ipart);
			if (frac)
				blocks[num_blocks - 1]++;
		}
		normalize();
		if (frac) {
			/* Note: this is only guaranteed to terminate
			 * (and be exact) in base-2 floats (i.e.,
			 * IEEE-754)
			 */
			decnumber<BT, ET> f(5), h(5);
			f.exponent = -1;
			h.exponent = -1;
			while (frac) {
				if (frac >= 0.5) {
					*this += f;
					frac -= 0.5;
				}
				frac *= 2;
				f *= h;
			}
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
		if (other.exponent <= exponent) {
			add = &other;
			while (add->exponent < exponent) {
				mul_unnorm(10);
				exponent--;
			}
		} else {
			tmp = other;
			while (exponent < tmp.exponent) {
				tmp.mul_unnorm(10);
				tmp.exponent--;
			}
			add = &tmp;
		}
		BT carry = 0;
		if (num_blocks < add->num_blocks) {
			size_t d = add->num_blocks - num_blocks;
			BT *tmp = new BT[add->num_blocks];
			memcpy(tmp + d, blocks, num_blocks * sizeof(*tmp));
			delete[] blocks;
			blocks = tmp;
			while (d--)
				blocks[d] = 0;
			num_blocks = add->num_blocks;
		}
		size_t d = num_blocks - add->num_blocks;
		for (std::size_t i = num_blocks - 1; i != SIZE_MAX; i--) {
			if (blocks[i] == (base - 1) && carry) {
				blocks[i] = 0;
				carry = 1;
			} else {
				blocks[i] += carry;
				carry = 0;
			}
			if (i >= d) {
				if (blocks[i] > base - add->blocks[i - d]) {
					carry = 1;
					blocks[i] -= base - add->blocks[i - d];
				} else {
					carry = 0;
					blocks[i] += add->blocks[i - d];
				}
			}
		}
		if (carry) {
			if (num_blocks == SIZE_MAX)
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
		if (other.exponent <= ret.exponent) {
			sub = &other;
			while (sub->exponent < ret.exponent) {
				ret.mul_unnorm(10);
				ret.exponent--;
			}
		} else {
			tmp = other;
			while (exponent < tmp.exponent) {
				tmp.mul_unnorm(10);
				tmp.exponent--;
			}
			sub = &tmp;
		}
		if (ret.num_blocks < sub->num_blocks) {
			size_t d = sub->num_blocks - ret.num_blocks;
			BT *tmp = new BT[sub->num_blocks];
			memcpy(tmp + d, ret.blocks, ret.num_blocks * sizeof(*tmp));
			delete[] ret.blocks;
			ret.blocks = tmp;
			while (d--)
				ret.blocks[d] = 0;
			ret.num_blocks = sub->num_blocks;
		}
		size_t d = ret.num_blocks - sub->num_blocks;
		BT carry = 0;
		for (std::size_t i = ret.num_blocks - 1; i != SIZE_MAX; i--) {
			if (ret.blocks[i] < carry) {
				ret.blocks[i] += base - carry;
				carry = 1;
			} else {
				ret.blocks[i] -= carry;
				carry = 0;
			}
			if (i >= d) {
				if (ret.blocks[i] < sub->blocks[i - d]) {
					carry += 1;
					ret.blocks[i] += base - sub->blocks[i - d];
				} else {
					ret.blocks[i] -= sub->blocks[i - d];
				}
			}
		}
		if (carry) {
			ret = other - *this;
			ret.sign = -1;
		} else {
			ret.normalize();
		}
		return ret;
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
		ET emin = std::numeric_limits<ET>::min();
		ET target_exponent = other.exponent;
		if ((target_exponent > 0 && exponent > emax - target_exponent)
		 || (target_exponent < 0 && exponent < emin - target_exponent))
		{
			throw std::overflow_error("Multiplication result too large: " + std::to_string(target_exponent) + " plus " + std::to_string(exponent));
		}
		target_exponent += exponent;

		decnumber<BT, ET> multiplier(0);
		multiplier.blocks = blocks;
		multiplier.exponent = 0;
		multiplier.num_blocks = num_blocks;
		num_blocks = 0;
		blocks = nullptr;
		exponent = 0;
		sign = 1;

		decnumber<BT, ET> multiplicand(other);
		multiplicand.exponent = 0;
		std::size_t i;
		for (i = multiplicand.num_blocks - 1; i != SIZE_MAX; i--) {
			decnumber<BT, ET> tmp(multiplier);
			tmp.mul_unnorm(multiplicand.blocks[i]);
			tmp.normalize();
			*this += tmp;
			multiplier.mul_unnorm(base);
			multiplier.normalize();
		}
		sign = target_sign;
		exponent += target_exponent;
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

	bool operator==(const decnumber<BT, ET> &other) const {
		if (&other == this)
			return true;
		if (other.num_blocks == 0 && num_blocks == 0)
			return true;
		if (other.num_blocks != num_blocks)
			return false;
		if (other.sign != sign)
			return false;
		if (other.exponent != exponent)
			return false;
		return !memcmp(other.blocks, blocks, num_blocks * sizeof(BT));
	}

	bool operator!=(const decnumber<BT, ET> &other) const {
		return !(*this == other);
	}

	bool operator<(const decnumber<BT, ET> &other) const {
		if (&other == this)
			return false;
		if (other.num_blocks == 0 && num_blocks == 0)
			return false;
		if (num_blocks == 0)
			return other.sign > 0;
		if (other.num_blocks == 0)
			return sign < 0;
		if (other.sign != sign)
			return sign < other.sign;
		if (other.exponent == exponent) {
			if (other.num_blocks != num_blocks)
				return num_blocks < other.num_blocks;
			for (size_t i = 0; i < num_blocks; i++)
				if (blocks[i] != other.blocks[i])
					return blocks[i] < other.blocks[i];
		}
		decnumber<BT, ET> delta = (*this) - other;
		if (delta.sign < 0)
			return true;
		return false;
	}

	bool operator<=(const decnumber<BT, ET> &other) const {
		return (*this == other) || (*this < other);
	}

	bool operator>(const decnumber<BT, ET> &other) const {
		return !(*this <= other);
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
		if (exponent == 1)
			ret += std::string("0");
		else if (exponent == 2)
			ret += std::string("00");
		else if (exponent > 2) {
			ret += std::string("e") + std::to_string(exponent);
		} else if (exponent < 0) {
			uintmax_t neg_exponent = -(uintmax_t)exponent;
			size_t digits = ret.size();
			size_t dot_pos;
			if (digits == neg_exponent) {
				digits++;
				neg_exponent = 0;
				ret = "0" + ret;
				dot_pos = 1;
			} else if (digits == neg_exponent - 1) {
				digits += 2;
				neg_exponent = 0;
				ret = "00" + ret;
				dot_pos = 1;
			} else if (digits == neg_exponent - 2) {
				digits += 3;
				neg_exponent = 0;
				ret = "000" + ret;
				dot_pos = 1;
			} else if (digits > neg_exponent) {
				dot_pos = digits - neg_exponent;
				neg_exponent = 0;
			} else {
				dot_pos = 1;
				neg_exponent -= digits - 1;
			}
			if (digits > 1) {
				ret += ".";
				while (digits > dot_pos) {
					ret[digits] = ret[digits - 1];
					ret[digits - 1] = '.';
					digits--;
				}
			}
			if (neg_exponent)
				ret += std::string("e-") + std::to_string(neg_exponent);
		}
		if (sign < 0)
			return "-" + ret;
		else
			return ret;
	}

	explicit operator double() const {
		return std::stod(str());
	}

	bool is_zero() const {
		return num_blocks == 0;
	}

	void divint(BT num) {
		if ((num < 0 && num < -base) || num > base)
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

#endif // DECNUMBER_H
