/*
  This file is part of Ingen.
  Copyright 2007-2016 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef INGEN_ENGINE_TASK_HPP
#define INGEN_ENGINE_TASK_HPP

#include <cassert>
#include <ostream>
#include <vector>

namespace Ingen {
namespace Server {

class BlockImpl;
class RunContext;

class Task : public std::vector<Task> {
public:
	enum class Mode {
		SINGLE,      ///< Single block to run
		SEQUENTIAL,  ///< Elements must be run sequentially in order
		PARALLEL     ///< Elements may be run in any order in parallel
	};

	Task(Mode mode, BlockImpl* block=NULL)
		: _block(block)
		, _mode(mode)
		, _done_end(0)
		, _next(0)
		, _done(false)
	{
		assert(!(mode == Mode::SINGLE && !block));
	}

	Task& operator=(const Task& copy) {
		*static_cast<std::vector<Task>*>(this) = copy;
		_block    = copy._block;
		_mode     = copy._mode;
		_done_end = copy._done_end;
		_next     = copy._next.load();
		_done     = copy._done.load();
		return *this;
	}

	Task(const Task& copy)
		: std::vector<Task>(copy)
		, _block(copy._block)
		, _mode(copy._mode)
		, _done_end(copy._done_end)
		, _next(copy._next.load())
		, _done(copy._done.load())
	{}

	/** Run task in the given context. */
	void run(RunContext& context);

	/** Pretty print task to the given stream (recursively). */
	void dump(std::function<void (const std::string&)> sink, unsigned indent, bool first) const;

	/** Simplify task expression. */
	void simplify();

	/** Steal a child task from this task (succeeds for PARALLEL only). */
	Task* steal(RunContext& context);

	Mode       mode()  const { return _mode; }
	BlockImpl* block() const { return _block; }
	bool       done()  const { return _done; }

	void set_done(bool done) { _done = done; }

private:
	Task* get_task(RunContext& context);

	BlockImpl*            _block;     ///< Used for SINGLE only
	Mode                  _mode;      ///< Execution mode
	unsigned              _done_end;  ///< Index of rightmost done sub-task
	std::atomic<unsigned> _next;      ///< Index of next sub-task
	std::atomic<bool>     _done;      ///< Completion phase
};

} // namespace Server
} // namespace Ingen

#endif // INGEN_ENGINE_TASK_HPP
