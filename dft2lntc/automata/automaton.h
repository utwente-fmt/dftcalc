#ifndef AUTOMATA_H
#define AUTOMATA_H

#include <set>
#include <ostream>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <iostream>

/* Warning to new automata:
 * Internal actions can be indicated by leaving the action empty.
 * However, these will be collapsed before the automaton is output.
 * The tau-reduction does not consider probabilistic behaviour (as
 * stochastic behaviour may not be known before the later renaming
 * step), and will produce incorrect results if a tau-action decides
 * what stochastic transitions will be enabled.
 */
class automaton {
public:
	class state {
	private:
		std::set<std::pair<std::string, size_t>> outgoing;
		automaton * parent;

	protected:
		state(automaton *parent)
			:parent(parent)
		{ }

		virtual void initialize_outgoing() = 0;
		void add_transition(std::string label, const state &target) {
			size_t target_num = parent->add_state(&target);
			outgoing.emplace(std::make_pair(label, target_num));
		}

	public:
		virtual size_t hashcode() const noexcept = 0;
		virtual bool operator==(const state &other) const noexcept = 0;
		virtual state *copy() const = 0;

		virtual ~state() { }

		virtual operator std::string() const {
			return "";
		}

		automaton *get_parent() {
			return parent;
		}

		const automaton *get_parent() const {
			return parent;
		}
		
		friend class automaton;
	};

	class stateHash {
	public:
		size_t operator()(const state * const &s) const {
			return s->hashcode();
		}
	};

	class ptr_equal {
	public:
		size_t operator()(const state * const &s1,
		                  const state * const &s2) const
		{
			return *s1 == *s2;
		}
	};

	virtual const state *initial_state() const = 0;

	void write(std::ostream &out);

	~automaton() {
		for (state *entry : states) {
			delete entry;
		}
	}

private:
	std::unordered_map<const state *, size_t, stateHash, ptr_equal> stateNums;
	std::vector<state *> states;

	size_t add_state(const state *newState) {
		auto existing = stateNums.find(newState);
		if (existing != stateNums.end())
			return existing->second;
		state *copy = newState->copy();
		copy->outgoing.clear();
		size_t ret = stateNums.size();
		stateNums.emplace(copy, ret);
		states.emplace_back(copy);
		return ret;
	}

	void tau_collapse();
};

#endif
