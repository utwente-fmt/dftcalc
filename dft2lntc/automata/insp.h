#ifndef AUTOMATA_H
#include "automata/automata.h"
#endif

#ifndef AUTOMATA_INSP_H
#define AUTOMATA_INSP_H
#include "dftnodes/Inspection.h"

namespace automata {
	class insp : public automaton {
		const size_t total;
		const size_t phases;

		class insp_state : public automaton::state {
			private:
			size_t counter;
			bool signal : 1;

			insp_state(insp *parent)
				:automaton::state(parent),
				 counter(0), signal(0)
			{ }

			friend class insp;

			public:
			size_t hashcode() const noexcept {
				return (counter << 1) + signal;
			}

			virtual state *copy() const {
				return new insp_state(*this);
			}

			bool operator==(const state &other) const noexcept {
				if (get_parent() != other.get_parent())
					return 0;
				const insp_state &o = static_cast<const insp_state &>(other);
				return counter == o.counter
				       && signal == o.signal;
			}

			virtual operator std::string() const;

			protected:
			virtual void initialize_outgoing();
		};

		insp_state initial_insp_state;

	public:
		insp(const DFT::Nodes::Inspection &gate)
			: total(gate.getChildren().size()),
			  phases(gate.getPhases() ? gate.getPhases() : 1),
			  initial_insp_state(this)
		{ }

		const state *initial_state() const {
			return &initial_insp_state;
		}
	};
};

#endif
