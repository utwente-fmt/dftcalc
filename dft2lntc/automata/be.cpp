#include "automata/be.h"

namespace automata {
using namespace signals;

static bool can_failsig(bool cold, bool isactive, enum be::BE_STATE status)
{
	return (status == be::BE_STATE::UP) && (isactive || !(cold || isactive));
}

void be::be_state::fail_sig(bool active,size_t phase,
                            bool canfail, be_state &target)
{
	if (canfail && phase == 1) {
		if (!active) {
			add_transition(RATE_FAIL(phase, 1), target);
			add_transition(RATE_FAIL(0, 1), target);
		} else {
			add_transition(RATE_FAIL(phase, 2), target);
			add_transition(RATE_FAIL(0, 2), target);
		}
	} else {
		if (!active)
			add_transition(RATE_FAIL(phase, 1), target);
		else
			add_transition(RATE_FAIL(phase, 2), target);
	}
}

void be::be_state::add_transition(std::string label, be_state &target)
{
	const be *par = (const be *)get_parent();
	if (target.repair_status == NEEDED && target.phase == 1) {
		if (target.emit_inspect || !par->repairable)
			target.status = IMPOSSIBLE;
		target.repair_status = DONE;
	}

	if (target.repair_status == BUSY && target.phase == 1) {
		if (target.emit_inspect || !par->repairable)
			target.status = IMPOSSIBLE;
		target.repair_status = DONE;
	}

	if (target.repair_status == DONE && par->independent_repair)
		target.repair_status = NONE;
	state::add_transition(label, target);
}

void be::be_state::initialize_outgoing() {
	if (terminated)
		return;

	be_state target(*this);
	if (status == IMPOSSIBLE) {
		target.terminated = 1;
		add_transition(GATE_IMPOSSIBLE, target);
		return;
	}

	target.isactive = 1;
	add_transition(ACTIVATE(0, false), target);
	const be *par = (const be *)get_parent();

	target = *this;
	if (par->always_active)
		target.status = IMPOSSIBLE;
	else
		target.isactive = false;
	add_transition(DEACTIVATE(0, false), target);

	if (can_failsig(par->cold, isactive, status)) {
		target = *this;
		target.can_definitely_fail = 1;
		if (phase == par->phases) {
			target.emit_fail = 1;
			target.status = DOWN;
		}
		if (phase == par->threshold)
			target.emit_inspect = true;
		target.phase++;
		fail_sig(isactive, phase, can_definitely_fail, target);
	}

	if (status == UP && phase == 1 && !can_definitely_fail) {
		if (!par->cold && !isactive) {
			target = *this;
			target.status = FAILSAFE;
			add_transition(RATE_FAIL(0, 1), target);
		}
		if (isactive) {
			target = *this;
			target.status = FAILSAFE;
			add_transition(RATE_FAIL(0, 2), target);
		}
	}

	if (emit_inspect) {
		target = *this;
		target.emit_inspect = 0;
		add_transition(INSPECT(0), target);
	}

	if (emit_fail) {
		target = *this;
		target.emit_fail = 0;
		add_transition(FAIL(0), target);
	} else if (emit_online) {
		target = *this;
		target.emit_online = 0;
		add_transition(ONLINE(0), target);
	}

	if (par->repairable && par->self_repair && status==DOWN
	    && repair_status==NONE)
	{
		target = *this;
		if (par->independent_repair)
			target.repair_status = BUSY;
		else
			target.repair_status = NEEDED;
		add_transition("", target);
	}

	target = *this;
	if (!par->repairable)
		target.status = IMPOSSIBLE;
	if (repair_status == NONE) {
		if (par->independent_repair)
			target.repair_status = BUSY;
		else
			target.repair_status = NEEDED;
	}
	add_transition(REPAIR(false), target);

	if (repair_status == NEEDED) {
		target = *this;
		target.repair_status == WAITING;
		add_transition(REPAIR(true), target);
	}

	target = *this;
	if (repair_status == WAITING)
		target.repair_status = BUSY;
	else
		target.status = IMPOSSIBLE;
	add_transition(REPAIRING(0), target);

	if (repair_status == BUSY) {
		target = *this;
		target.phase = 1;
		target.repair_status = DONE;
		if (status == DOWN)
			target.emit_online = true;
		if (status != FAILSAFE)
			target.status = UP;
		if (emit_inspect)
			target.status = IMPOSSIBLE;
		add_transition(GATE_RATE_REPAIR, target);
	}

	if (repair_status == DONE) {
		target = *this;
		target.repair_status = NONE;
		add_transition(REPAIRED(0), target);
	}
}

be::be_state::operator std::string() const {
	if (terminated)
		return "[]";
	if (status == IMPOSSIBLE)
		return "[IMPOSSIBLE]";
	std::string ret = "[";
	switch (status) {
	case UP: ret += "UP"; break;
	case FAILING: ret += "FAILING"; break;
	case DOWN: ret += "DOWN"; break;
	case FAILSAFE: ret += "FAILSAFE"; break;
	case IMPOSSIBLE: ret += "IMPOSSIBLE"; break;
	default: ret += "UNKNOWN"; break;
	}
	ret += ", ";
	switch (repair_status) {
	case NONE: ret += "NONE"; break;
	case BUSY: ret += "BUSY"; break;
	case NEEDED: ret += "NEEDED"; break;
	case WAITING: ret += "WAITING"; break;
	case DONE: ret += "DONE"; break;
	default: ret += "UNKNOWN"; break;
	}
	ret += ", ";
	ret += std::to_string(phase);
	ret += ", ";
	ret += isactive ? "active" : ", inactive";
	if (emit_fail)
		ret += ", emit_fail";
	if (emit_online)
		ret += ", emit_online";
	if (emit_inspect)
		ret += ", emit_inspect";
	if (can_definitely_fail)
		ret += ", can_definitely_fail";
	ret += "]";
	return ret;
}

} /* Namespace automata */
