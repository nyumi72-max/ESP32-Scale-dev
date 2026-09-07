#include "CommandRouter.h"

CommandRouter::CommandRouter()
{
    _handlerCount = 0;
    _guard = nullptr;
}

void CommandRouter::setGuard(
    CommandGuard guard)
{
    _guard = guard;
}

bool CommandRouter::addHandler(
    CommandHandler handler)
{
    if (_handlerCount >= MAX_HANDLERS)
    {
        return false;
    }

    _handlers[_handlerCount] = handler;
    _handlerCount++;

    return true;
}

bool CommandRouter::dispatch(
    const String& cmd)
{
    if (_guard && !_guard(cmd))
    {
        return true;
    }

    for (int i = 0; i < _handlerCount; i++)
    {
        if (_handlers[i](cmd))
        {
            return true;
        }
    }

    return false;
}
