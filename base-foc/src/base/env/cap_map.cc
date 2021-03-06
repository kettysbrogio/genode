/*
 * \brief  Mapping of Genode's capability names to kernel capabilities.
 * \author Stefan Kalkowski
 * \date   2010-12-06
 *
 * This is a Fiasco.OC-specific addition to the process enviroment.
 */

/*
 * Copyright (C) 2010-2012 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU General Public License version 2.
 */

#include <base/cap_map.h>
#include <base/native_types.h>

#include <util/assert.h>

/* Lock implementation local include */
#include <spin_lock.h>

namespace Fiasco {
#include <l4/sys/consts.h>
#include <l4/sys/task.h>
}


/***********************
 **  Cap_index class  **
 ***********************/

static volatile int _cap_index_spinlock = SPINLOCK_UNLOCKED;


bool Genode::Cap_index::higher(Genode::Cap_index *n) { return n->_id > _id; }


Genode::Cap_index* Genode::Cap_index::find_by_id(Genode::uint16_t id)
{
	using namespace Genode;

	if (_id == id) return this;

	Cap_index *n = Avl_node<Cap_index>::child(id > _id);
	return n ? n->find_by_id(id) : 0;
}


Genode::addr_t Genode::Cap_index::kcap() {
	return cap_idx_alloc()->idx_to_kcap(this); }


Genode::uint8_t Genode::Cap_index::inc()
{
	spinlock_lock(&_cap_index_spinlock);
	Genode::uint8_t ret = ++_ref_cnt;
	spinlock_unlock(&_cap_index_spinlock);
	return ret;
}


Genode::uint8_t Genode::Cap_index::dec()
{
	spinlock_lock(&_cap_index_spinlock);
	Genode::uint8_t ret = --_ref_cnt;
	spinlock_unlock(&_cap_index_spinlock);
	return ret;
}


/****************************
 **  Capability_map class  **
 ****************************/

Genode::Cap_index* Genode::Capability_map::find(int id)
{
	using namespace Genode;

	Lock_guard<Spin_lock> guard(_lock);

	Cap_index* i = 0;
	if (_tree.first())
		i = _tree.first()->find_by_id(id);
	return i;
}


Genode::Cap_index* Genode::Capability_map::insert(int id)
{
	using namespace Genode;

	Lock_guard<Spin_lock> guard(_lock);

	ASSERT(!_tree.first() || !_tree.first()->find_by_id(id),
	       "Double insertion in cap_map()!");

	Cap_index *i = cap_idx_alloc()->alloc(1);
	if (i) {
		i->id(id);
		_tree.insert(i);
	}
	return i;
}


Genode::Cap_index* Genode::Capability_map::insert(int id, addr_t kcap)
{
	using namespace Genode;

	Lock_guard<Spin_lock> guard(_lock);

	ASSERT(!_tree.first() || !_tree.first()->find_by_id(id),
	       "Double insertion in cap_map()!");

	Cap_index *i = cap_idx_alloc()->alloc(kcap, 1);
	if (i) {
		i->id(id);
		_tree.insert(i);
	}
	return i;
}


Genode::Cap_index* Genode::Capability_map::insert_map(int id, addr_t kcap)
{
	using namespace Genode;
	using namespace Fiasco;

	Lock_guard<Spin_lock> guard(_lock);

	Cap_index* i = 0;

	/* check whether capability id exists */
	if (_tree.first())
		i = _tree.first()->find_by_id(id);

	/* if we own the capability already check whether it's the same */
	if (i) {
		l4_msgtag_t tag = l4_task_cap_equal(L4_BASE_TASK_CAP, i->kcap(), kcap);
		if (!l4_msgtag_label(tag)) {
			/*
			 * they aren't equal, possibly an already revoked cap,
			 * otherwise it's a fake capability and we return an invalid one
			 */
			tag = l4_task_cap_valid(L4_BASE_TASK_CAP, i->kcap());
			if (l4_msgtag_label(tag))
				return 0;
		} else
			/* they are equal so just return the one in the map */
			return i;
	} else {
		/* the capability doesn't exists in the map so allocate a new one */
		i = cap_idx_alloc()->alloc(1);
		if (!i)
			return 0;
		i->id(id);
		_tree.insert(i);
	}

	/* map the given cap to our registry entry */
	l4_task_map(L4_BASE_TASK_CAP, L4_BASE_TASK_CAP,
				l4_obj_fpage(kcap, 0, L4_FPAGE_RWX),
				i->kcap() | L4_ITEM_MAP | L4_MAP_ITEM_GRANT);
	return i;
}


void Genode::Capability_map::remove(Genode::Cap_index* i)
{
	using namespace Genode;

	Lock_guard<Spin_lock> guard(_lock);

	if (i) {
		Cap_index* e = _tree.first() ? _tree.first()->find_by_id(i->id()) : 0;
		if (e == i)
			_tree.remove(i);
		cap_idx_alloc()->free(i, 1);
	}
}


Genode::Capability_map* Genode::cap_map()
{
	static Genode::Capability_map map;
	return &map;
}
