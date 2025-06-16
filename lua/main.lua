local color = require 'color'
local SimpleTask = require 'simpletask'

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
    config = config
  }
  print('meta')
  print(Context)
  setmetatable(result, Context)
  return result
end

function make_task(config)
  local context = Context.new(config)
  print('context')
  print(context)
  local q = getmetatable(context)
  print(q)
  local task_type = config.task_type
  if task_type == 'simple' then
    return SimpleTask.new(context)
  end
end
