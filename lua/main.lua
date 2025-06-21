local color = require 'color'
local SimpleTask = require 'simpletask'

function thalamus.wait(condition, timeout)
  local sleep
  if timeout == nil then
    sleep = function() return false end
  else
    sleep = thalamus.sleep(timeout)
  end

  coroutine.yield(function()
    return condition() or sleep()
  end)
  return condition()
end

function thalamus.hold(is_held, hold, blink)
  local start = thalamus.clock()
  local time_blinking = 0.0
  while true do
    local elapsed = thalamus.clock() - start
    local remaining = hold - elapsed + time_blinking
    if remaining < 0 then
      break
    end
    local blinked = thalamus.wait(function() return not is_held() end, remaining)
    if not blinked then
      break
    end

    local t0 = thalamus.clock()
    local reacquired = thalamus.wait(is_held, blink)
    if not reacquired then
      return false
    end
    local t1 = thalamus.clock()
    time_blinking = time_blinking + t1 - t0
  end
  return true
end

function test()
  print(1)
  coroutine.yield(thalamus.sleep(1.0))
  print(1)
  coroutine.yield(thalamus.sleep(1.0))
  print(1)
  coroutine.yield(thalamus.sleep(1.0))

  local sleep = thalamus.sleep(1.0) 
  coroutine.yield(function()
    return sleep and true
  end)
  print(1)
end

local Context = {}
Context.__index = Context

function get_uniform(config)
  local min = config.min
  local max = config.max
  local range = max-min
  return min + math.random()*range
end

function get_color(config)
  return color.SetRGB(config[1], config[2], config[3])
end

function Context:get_uniform_field(field)
  return get_uniform(self.config[field]);
end

function Context:get_color_field(field)
  return get_color(self.config[field]);
end

function Context.new(config)
  local result = {
    config = config,
    log = thalamus_log
  }
  print('meta')
  print(Context)
  setmetatable(result, Context)
  return result
end

function make_task(config)
  local context = Context.new(config)
  local task_type = config.task_type
  local task = nil
  if task_type == 'simple' then
    task = coroutine.create(function() return SimpleTask.run(context) end)
  end
  return task, context
end
