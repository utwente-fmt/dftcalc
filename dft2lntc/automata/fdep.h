#ifndef AUTOMATA_H
#include "automata/automata.h"
#endif

#ifndef AUTOMATA_FDEP_H
#define AUTOMATA_FDEP_H
#include "dftnodes/GateFDEP.h"
#include <vector>

namespace automata {
	class fdep : public automaton {
		const size_t total;
		const bool trigger_always_active : 1;

		class fdep_state : public automaton::state {
			private:
			size_t act;
			std::vector<bool> notified;
			bool failed : 1;

			fdep_state(fdep *parent)
				:automaton::state(parent),
				 act(0),
				 notified(parent->total)
			{ }

			friend class fdep;

			public:
			size_t hashcode() const noexcept {
				size_t ret = act << 1;
				ret += failed;
				ret *= 33;
				ret += std::hash<std::vector<bool>>()(notified);
				return ret;
			}

			virtual state *copy() const {
				return new fdep_state(*this);
			}

			bool operator==(const state &other) const noexcept {
				if (get_parent() != other.get_parent())
					return 0;
				const fdep_state &o = static_cast<const fdep_state &>(other);
				return act == o.act
				       && notified == o.notified
				       && failed == o.failed;
			}

			virtual operator std::string() const;

			protected:
			virtual void initialize_outgoing();
		};

		fdep_state initial_fdep_state;

	public:
		fdep(const DFT::Nodes::GateFDEP &gate)
			: total(gate.getDependers().size()),
			  trigger_always_active(gate.getChildren()[0]->isAlwaysActive()),
			  initial_fdep_state(this)
		{ }

		const state *initial_state() const {
			return &initial_fdep_state;
		}
	};
};

#endif
