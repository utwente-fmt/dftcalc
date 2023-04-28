#ifndef AUTOMATA_SIGNALS_H
#define AUTOMATA_SIGNALS_H
#include <string>

namespace automata {
	namespace signals {
		static inline const std::string GATE_FAIL("FAIL");
		static inline const std::string GATE_ACTIVATE("ACTIVATE");
		static inline const std::string GATE_DEACTIVATE("DEACTIVATE");
		/* REPAIR?: Insp to BE: Start repairing
		 * REPAIR!: BE to RU: In need of repair.
		 */
		static inline const std::string GATE_REPAIR("REPAIR");
		/* REPAIRED: BE to RU: Repair finished */
		static inline const std::string GATE_REPAIRED("REPAIRED");
		static inline const std::string GATE_ONLINE("ONLINE");
		static inline const std::string GATE_RATE_FAIL("RATE_FAIL");
		static inline const std::string GATE_RATE_PERIOD("RATE_PERIOD");
		static inline const std::string GATE_RATE_REPAIR("RATE_REPAIR");
		static inline const std::string GATE_RATE_INSPECTION("RATE_INSPECTION");
		/* REPAIRING: RU to BE: Start repair time */
		static inline const std::string GATE_REPAIRING("REPAIRING");
		static inline const std::string GATE_INSPECT("INSPECT");
		static inline const std::string GATE_IMPOSSIBLE("IMPOSSIBLE");

		static inline std::string ACTIVATE(size_t num, bool direction) {
			std::string ret = GATE_ACTIVATE + " !";
			ret += std::to_string(num);
			ret += " !";
			ret += direction ? "TRUE" : "FALSE";
			return ret;
		}

		static inline std::string DEACTIVATE(size_t num, bool direction) {
			std::string ret = GATE_DEACTIVATE + " !";
			ret += std::to_string(num);
			ret += " !";
			ret += direction ? "TRUE" : "FALSE";
			return ret;
		}

		static inline std::string FAIL(size_t num) {
			std::string ret = GATE_FAIL + " !";
			ret += std::to_string(num);
			return ret;
		}

		static inline std::string ONLINE(size_t num) {
			std::string ret = GATE_ONLINE + " !";
			ret += std::to_string(num);
			return ret;
		}

		static inline std::string RATE_FAIL(size_t num1, size_t num2) {
			std::string ret = GATE_RATE_FAIL + " !";
			ret += std::to_string(num1);
			ret += " !";
			ret += std::to_string(num2);
			return ret;
		}

		static inline std::string RATE_INSPECTION(size_t num) {
			std::string ret = GATE_RATE_INSPECTION + " !";
			ret += std::to_string(num);
			return ret;
		}

		static inline std::string INSPECT(size_t num) {
			std::string ret = GATE_INSPECT + " !";
			ret += std::to_string(num);
			return ret;
		}

		static inline std::string REPAIR(bool direction) {
			std::string ret = GATE_REPAIR + " !";
			ret += direction ? "TRUE" : "FALSE";
			return ret;
		}

		static inline std::string REPAIR(int num) {
			std::string ret = GATE_REPAIR + " !";
			ret += std::to_string(num);
			return ret;
		}

		static inline std::string REPAIRING(size_t num) {
			std::string ret = GATE_REPAIRING + " !";
			ret += std::to_string(num);
			return ret;
		}

		static inline std::string REPAIRED(size_t num) {
			std::string ret = GATE_REPAIRED + " !";
			ret += std::to_string(num);
			return ret;
		}
	};
};

#endif
