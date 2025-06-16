local HoldStateMachine = {}
HoldStateMachine .__index = HoldStateMachine 

function HoldStateMachine.new(hold, blink)
  local result = {
    hold=hold or 0,
    blink=blink or 0,
    blink_start=nil,
    start=os.clock(),
    blink_time = 0.0
  }
  setmetatable(result, HoldStateMachine)
  return result
end

function HoldStateMachine:lost()
  self.blink_start = os.clock()
end

function HoldStateMachine:acquired(value)
  if value == nil then
    return self:acquired(true)
  end

  if not value then
    return self:lost()
  end

  if not self.blink_start then
    return
  end

  local now = os.clock()
  self.blink_time = self.blink_time + now - self.blink_start
  self.blink_start = nil
end

function HoldStateMachine:update()
  local now = os.clock()
  --print((now - self.start) .. (self.hold + self.blink_time))
  if self.blink_start ~= nil then
    local current_blink_time = now - self.blink_start
    print(current_blink_time .. ' ' .. self.blink)
    if current_blink_time > self.blink then
      return "FAIL"
    end
  elseif now - self.start > self.hold + self.blink_time then
    return "SUCCESS"
  end
  return "PENDING"
end

return HoldStateMachine
