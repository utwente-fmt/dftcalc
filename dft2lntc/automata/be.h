#ifndef AUTOMATA_H
#include "automata/automata.h"
#endif

#ifndef AUTOMATA_BE_H
#define AUTOMATA_BE_H
#include "dftnodes/BasicEvent.h"

namespace automata {
	class be : public automaton {
	public:
		enum BE_STATE {
			UP, FAILING, DOWN, FAILSAFE, IMPOSSIBLE
		};

	private:
		enum REPAIR_STATE {
			NONE, BUSY, NEEDED, WAITING, DONE
		};
		const size_t phases;
		const size_t threshold;
		const bool cold : 1;
		const bool repairable : 1;
		const bool self_repair : 1;
		const bool independent_repair : 1;
		const bool always_active : 1;
		const bool has_res : 1;

		class be_state : public automaton::state {
			private:
			enum BE_STATE status;
			enum REPAIR_STATE repair_status;
			size_t phase;
			bool isactive : 1;
			bool emit_fail : 1;
			bool emit_online : 1;
			bool emit_inspect : 1;
			bool can_definitely_fail : 1;
			bool terminated : 1;

			be_state(be *parent, BE_STATE status)
				:automaton::state(parent),
				 status(status),
				 repair_status(NONE),
				 phase(1),
				 isactive(parent->always_active),
				 emit_fail(false), emit_online(false),
				 emit_inspect(false),
				 can_definitely_fail(false),
				 terminated(false)
			{
				if (status == FAILING) {
					emit_fail = 1;
					if (parent->threshold >= 0)
						emit_inspect = 1;
					status = UP;
				}
			}

			void fail_sig(bool active, size_t phase, bool canfail, be_state &target);
			void fail_res(bool active, size_t phase, bool canfail, be_state &target);
			void add_transition(std::string label, be_state &target);
			friend class be;

			public:
			size_t hashcode() const noexcept {
				if (terminated)
					return 0;
				if (status == IMPOSSIBLE)
					return 1;
				size_t ret = status;
				ret = (ret * 33) + repair_status;
				ret = (ret * 33) + phase;
				ret = (ret * 33) + isactive;
				ret = (ret * 33) + emit_fail;
				ret = (ret * 33) + emit_online;
				ret = (ret * 33) + emit_inspect;
				ret = (ret * 33) + can_definitely_fail;
				return ret;
			}

			virtual state *copy() const {
				return new be_state(*this);
			}

			bool operator==(const state &other) const noexcept {
				if (get_parent() != other.get_parent())
					return 0;
				const be_state &o = static_cast<const be_state &>(other);
				if (terminated)
					return o.terminated;
				if (o.terminated)
					return 0;
				if (status == IMPOSSIBLE)
					return o.status == IMPOSSIBLE;
				if (o.status == IMPOSSIBLE)
					return 0;
				return status == o.status
				       && repair_status == o.repair_status
				       && phase == o.phase
				       && isactive == o.isactive
				       && emit_fail == o.emit_fail
				       && emit_online == o.emit_online
				       && emit_inspect == o.emit_inspect
				       && can_definitely_fail == o.can_definitely_fail
				       && terminated == o.terminated;
			}

			virtual operator std::string() const;

			protected:
			virtual void initialize_outgoing();
		};

		be_state initial_be_state;

	public:
		be(const DFT::Nodes::BasicEvent &be)
			: phases(be.getPhases()),
			  threshold(be.getInterval()),
			  cold(be.getMu().is_zero()),
			  repairable(be.isRepairable()),
			  self_repair(!be.hasInspectionModule()),
			  independent_repair(!be.hasRepairModule()),
			  always_active(be.isAlwaysActive()),
			  has_res(be.getRes() != 1),
			  initial_be_state(this,
			                   be.getFailed() ? FAILING : UP)
		{
		}

		const state *initial_state() const {
			return &initial_be_state;
		}
	};
};

#endif
