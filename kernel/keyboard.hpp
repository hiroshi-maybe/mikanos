#pragma once

#include <cstdint>
#include <deque>

#include "message.hpp"

void InitializeKeyboard(std::deque<Message>& msg_queue);
