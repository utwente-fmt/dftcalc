#ifndef AUTOMATA_H
#include "automata/automata.h"
#endif

#ifndef AUTOMATA_PAND_H
#define AUTOMATA_PAND_H
#include "dftnodes/GatePAnd.h"
#include <vector>

namespace automata {
	class pand : public automaton {
		const size_t total;
		const bool repairable : 1;
		const bool always_active : 1;

		class pand_state : public automaton::state {
			private:
			std::vector<bool> failed;
			size_t nr_failed = 0;
			size_t nr_active = 0;
			size_t in_order_mark = 0;
			bool done : 1;
			bool impossible : 1;
			bool terminated : 1;

			pand_state(pand *parent)
				:automaton::state(parent),
				 failed(parent->total + 1),
				 nr_failed(0), nr_active(0), in_order_mark(0),
				 done(0), impossible(0), terminated(0)
			{ }

			friend class pand;

			public:
			size_t hashcode() const noexcept {
				if (terminated)
					return 0;
				if (impossible)
					return 1;
				size_t ret = 2;
				ret = (ret * 33) + nr_failed;
				ret = (ret * 33) + nr_active;
				ret = (ret * 33) + in_order_mark;
				ret = (ret * 33) + done;
				ret = (ret * 33) + std::hash<std::vector<bool>>()(failed);
				return ret;
			}

			virtual state *copy() const {
				return new pand_state(*this);
			}

			bool operator==(const state &other) const noexcept {
				if (get_parent() != other.get_parent())
					return 0;
				const pand_state &o = static_cast<const pand_state &>(other);
				if (terminated)
					return o.terminated;
				if (o.terminated)
					return 0;
				if (impossible)
					return o.impossible;
				if (o.impossible)
					return 0;
				return nr_failed == o.nr_failed
				       && nr_active == o.nr_active
				       && in_order_mark == o.in_order_mark
				       && done == o.done
				       && failed == o.failed;
			}

			virtual operator std::string() const;

			protected:
			virtual void initialize_outgoing();
		};

		pand_state initial_pand_state;

	public:
		pand(const DFT::Nodes::GatePAnd &gate)
			: total(gate.getChildren().size()),
			  repairable(total),
			  always_active(gate.isAlwaysActive()),
			  initial_pand_state(this)
		{
		}

		const state *initial_state() const {
			return &initial_pand_state;
		}
	};
};

#endif
