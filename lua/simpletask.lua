local HoldStateMachine = require 'hold_state_machine'
local FontManager = require 'fontmanager'
local FontStyle = require 'fontstyle'
local Font = require 'font'
local Rect = require 'rect'
local Paint = require 'paint'
local Color = require 'color'
local SimpleTask = {}

local fontMgr = FontManager.new()
local typeface = fontMgr:matchFamilyStyle(nil, FontStyle.new())
local font = Font.new(typeface, 48)
local paint = Paint.new(Color.Color4f.FromColor(Color.RED))

function SimpleTask.run(context)
  local intertrial_timeout = context:get_uniform_field("intertrial_timeout")
  local start_timeout = context:get_uniform_field("start_timeout")
  local hold_timeout = context:get_uniform_field("hold_timeout")
  local success_timeout = context:get_uniform_field("success_timeout")
  local fail_timeout = context:get_uniform_field("fail_timeout")
  local blink_timeout = context:get_uniform_field("blink_timeout")
  local target_x = context:get_uniform_field("target_x");
  local target_y = context:get_uniform_field("target_y");
  local target_width = context:get_uniform_field("target_width");
  local target_height = context:get_uniform_field("target_height");
  local target = Rect.MakeXYWH(target_x, target_y, target_width, target_height);
  local corner_rect = Rect.MakeXYWH(1720, 880, 200, 200);
  local target_color = context:get_color_field("target_color");
  local target_paint = Paint.new(Color.Color4f.FromColor(target_color))
  local state = ''
  local start = thalamus.clock()

  local set_state = function(new_state)
    state = new_state
    local message = 'BehavState=' .. state
    thalamus.log(message)
    print(message)
  end

  local x = 0
  local y = 0
  context.on_touch = function(_x, _y)
    x = _x
    y = _y
  end

  context.on_render = function(canvas, view)
    if state == 'START_ON' or state == 'START_ACQ' then
      canvas:drawRect(target, target_paint);
    end
    if state == 'START_ON' then
      canvas:drawRect(corner_rect, target_paint);
    end
    if view == 'OPERATOR' then
      local now = thalamus.clock()
      local duration = now - start;
      local text = view .. ' ' .. duration
      canvas:drawString(text, 0, 50, font, paint);
    end
  end

  while true do
    set_state('INTERTRIAL')
    local touched = thalamus.wait(function()
      return x > 0 and y > 0
    end, intertrial_timeout)
    if touched then
      set_state('FAIL')
      coroutine.yield(thalamus.sleep(fail_timeout))
      return false
    end

    set_state('START_ON')
    touched = thalamus.wait(function ()
      return x > 0 and y > 0
    end, start_timeout)

    if touched then
      if target:contains(x, y) then
        break
      else
        set_state('FAIL')
        coroutine.yield(thalamus.sleep(fail_timeout))
        return false
      end
    end

  end

  set_state('START_ACQ')
  local held = thalamus.hold(function() return target:contains(x, y) end, hold_timeout, blink_timeout)
  if held then
    set_state('SUCCESS')
    coroutine.yield(thalamus.sleep(success_timeout))
    return false
  else
    set_state('FAIL')
    coroutine.yield(thalamus.sleep(fail_timeout))
    return false
  end

  return true
end

return SimpleTask
