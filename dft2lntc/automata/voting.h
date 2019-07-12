#ifndef AUTOMATA_H
#include "automata/automata.h"
#endif

#ifndef AUTOMATA_VOTING_H
#define AUTOMATA_VOTING_H
#include "dftnodes/GateOr.h"
#include "dftnodes/GateAnd.h"
#include "dftnodes/GateVoting.h"
#include <vector>

namespace automata {
	class voting : public automaton {
		const size_t total;
		const size_t threshold;
		const size_t repairable;
		const bool always_active : 1;

		class voting_state : public automaton::state {
			private:
			size_t nr_failed;
			bool done : 1;
			bool running : 1;
			bool impossible : 1;
			bool terminated : 1;
			std::vector<bool> received;
			std::vector<bool> activated;

			voting_state(voting *parent)
				:automaton::state(parent),
				 received(parent->total + 1),
				 activated(parent->total + 1)
			{ }

			friend class voting;

			public:
			size_t hashcode() const noexcept {
				if (terminated)
					return 0;
				if (impossible)
					return 1;
				size_t ret = 2;
				ret = (ret * 33) + nr_failed;
				ret = (ret * 33) + done;
				ret = (ret * 33) + running;
				ret = (ret * 33) + std::hash<std::vector<bool>>()(received);
				ret = (ret * 33) + std::hash<std::vector<bool>>()(activated);
				return ret;
			}

			virtual state *copy() const {
				return new voting_state(*this);
			}

			bool operator==(const state &other) const noexcept {
				if (get_parent() != other.get_parent())
					return 0;
				const voting_state &o = static_cast<const voting_state &>(other);
				if (terminated)
					return o.terminated;
				if (o.terminated)
					return 0;
				if (impossible)
					return o.impossible;
				if (o.impossible)
					return 0;
				return nr_failed == o.nr_failed
				       && done == o.done
				       && running == o.running
				       && received == o.received
				       && activated == o.activated;
			}

			virtual operator std::string() const;

			protected:
			virtual void initialize_outgoing();
		};

		voting_state initial_voting_state;

	public:
		voting(const DFT::Nodes::GateOr &gate)
			: total(gate.getChildren().size()),
			  threshold(1),
			  repairable(total),
			  always_active(gate.isAlwaysActive()),
			  initial_voting_state(this)
		{
		}

		voting(const DFT::Nodes::GateAnd &gate)
			: total(gate.getChildren().size()),
			  threshold(gate.getChildren().size()),
			  repairable(total),
			  always_active(gate.isAlwaysActive()),
			  initial_voting_state(this)
		{
		}

		voting(const DFT::Nodes::GateVoting &gate)
			: total(gate.getChildren().size()),
			  threshold(gate.getThreshold()),
			  repairable(total),
			  always_active(gate.isAlwaysActive()),
			  initial_voting_state(this)
		{
		}

		const state *initial_state() const {
			return &initial_voting_state;
		}
	};
};

#endif
