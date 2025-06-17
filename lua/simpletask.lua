local HoldStateMachine = require 'hold_state_machine'
local FontManager = require 'fontmanager'
local FontStyle = require 'fontstyle'
local Font = require 'font'
local Rect = require 'rect'
local Paint = require 'paint'
local Color = require 'color'
local SimpleTask = {}
SimpleTask.__index = SimpleTask

local fontMgr = FontManager.new()
local typeface = fontMgr:matchFamilyStyle(nil, FontStyle.new())
local font = Font.new(typeface, 48)
local paint = Paint.new(Color.Color4f.FromColor(Color.RED))

function SimpleTask.new(context)
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

  local result = {
    context = context,
    state = 'NONE',
    target = target,
    corner_rect = corner_rect,
    target_paint = target_paint,
    hold = HoldStateMachine.new(hold_timeout, blink_timeout),
    intertrial_timeout = intertrial_timeout,
    start_timeout = start_timeout,
    hold_timeout = hold_timeout,
    success_timeout = success_timeout,
    fail_timeout = fail_timeout,
    blink_timeout = blink_timeout,
    start = os.clock(),
    _status = nil
  }
  setmetatable(result, SimpleTask)
  return result
end

function SimpleTask:set_state(state)
  self.start = os.clock()
  self.state = state
  self.context.log('BehavState=' .. state)
end

function SimpleTask:touch(x, y)
  if self.state == 'INTERTRIAL' then
    if 0 <= x and 0 <= y then
      self:set_state('FAIL')
    end
  elseif self.state == 'START_ON' then
    if self.target:contains(x, y) then
      self.hold = HoldStateMachine.new(self.hold_timeout, self.blink_timeout)
      self:set_state('START_ACQ')
    elseif 0 <= x and 0 <= y then
      self:set_state('FAIL')
    end
  elseif self.state == 'START_ACQ' then
    self.hold:acquired(self.target:contains(x, y))
  end
end

function SimpleTask:gaze(x, y)
end

function SimpleTask:update()
  --print(self.state)
  if self.state == 'NONE' then
    self:set_state('INTERTRIAL')
  elseif self.state == 'INTERTRIAL' then
    local now = os.clock()
    if now - self.start > self.intertrial_timeout then
      self:set_state('START_ON')
    end
  elseif self.state == 'START_ON' then
    local now = os.clock()
    if now - self.start > self.start_timeout then
      self:set_state('INTERTRIAL')
    end
  elseif self.state == 'START_ACQ' then
    local status = self.hold:update()
    if status == 'SUCCESS' then
      self:set_state('SUCCESS')
    elseif status == 'FAIL' then
      self:set_state('FAIL')
    end
  elseif self.state == 'SUCCESS' then
    local now = os.clock()
    if now - self.start > self.success_timeout then
      self._status = {success = true}
    end
  elseif self.state == 'FAIL' then
    local now = os.clock()
    if now - self.start > self.fail_timeout then
      self._status = {success = false}
    end
  end
end

function SimpleTask:draw(canvas, view)
  if self.state == 'START_ON' or self.state == 'START_ACQ' then
    canvas:drawRect(self.target, self.target_paint);
  end
  if self.state == 'START_ON' then
    canvas:drawRect(self.corner_rect, self.target_paint);
  end
  if view == 'OPERATOR' then
    local now = os.clock()
    local duration = now - self.start;
    local text = view .. ' ' .. duration
    canvas:drawString(text, 0, 50, font, paint);
  end
end

function SimpleTask:status()
  return self._status
end

return SimpleTask
