#include <LSP/Threading/TaskDispatcher.hpp>

#include <LSP/Debugging/Logging.hpp>

using namespace LSP;

void TaskDispatcher::enqueue(Task&& task, const std::vector<TaskId>& depends)
{
	lsp_debug_log(task.id().id() << L" <= "; for(auto& dep:depends) _ << dep.id());
}