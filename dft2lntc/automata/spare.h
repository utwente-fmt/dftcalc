#ifndef AUTOMATA_H
#include "automata/automata.h"
#endif

#ifndef AUTOMATA_SPARE_H
#define AUTOMATA_SPARE_H
#include "dftnodes/GateWSP.h"
#include <set>

namespace automata {
	class spare : public automaton {
		const size_t total;
		bool repairable : 1;
		const bool always_active : 1;

		class spare_state : public automaton::state {
			private:
			set<size_t> unfailed;
			set<size_t> unclaimed;
			size_t repairing_deactivate;
			size_t cur_using;
			size_t prev_using;
			bool done : 1;
			bool activated : 1;
			bool impossible : 1;
			bool terminated : 1;

			spare_state(spare *parent)
				:automaton::state(parent),
				 repairing_deactivate(0),
				 cur_using(parent->always_active ? 1 : 0),
				 prev_using(0), done(0),
				 activated(parent->always_active),
				 impossible(0), terminated(0)
			{
				for (size_t i = 1; i <= parent->total; i++) {
					unfailed.insert(i);
					unclaimed.insert(i);
				}
			}

			void add_transition(std::string label, spare_state &target);
			friend class spare;

			public:
			size_t hashcode() const noexcept {
				if (terminated)
					return 0;
				if (impossible)
					return 1;
				size_t ret = cur_using;
				ret = (ret * 33) + repairing_deactivate;
				ret = (ret * 33) + prev_using;
				ret = (ret * 33) + done;
				ret = (ret * 33) + activated;
				for (size_t i : unfailed)
					ret = (ret * 33) + i;
				for (size_t i : unclaimed)
					ret = (ret * 31) + i;
				return ret;
			}

			virtual state *copy() const {
				return new spare_state(*this);
			}

			bool operator==(const state &other) const noexcept {
				if (get_parent() != other.get_parent())
					return 0;
				const spare_state &o = static_cast<const spare_state &>(other);
				if (terminated)
					return o.terminated;
				if (o.terminated)
					return 0;
				if (impossible)
					return o.impossible;
				if (o.impossible)
					return 0;
				return done == o.done
				       && repairing_deactivate == o.repairing_deactivate
				       && cur_using == o.cur_using
				       && prev_using == o.prev_using
				       && activated == o.activated
				       && unfailed == o.unfailed
				       && unclaimed == o.unclaimed;
			}

			virtual operator std::string() const;

			protected:
			virtual void initialize_outgoing();
		};

		spare_state initial_spare_state;

	public:
		spare(const DFT::Nodes::GateWSP &gate)
			: total(gate.getChildren().size()),
			  always_active(gate.isAlwaysActive()),
			  initial_spare_state(this)
		{
			for (const auto *child : gate.getChildren()) {
				if (child->isRepairable()) {
					repairable = true;
					break;
				}
			}
		}

		const state *initial_state() const {
			return &initial_spare_state;
		}
	};
};

#endif
