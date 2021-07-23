# luahttp
一个提供给lua使用的http协议解析库。

# 依赖
- [lua](https://github.com/xiyoo0812/lua.git)5.2以上
- 项目路径如下<br>
  |--proj <br>
  &emsp;|--lua <br>
  &emsp;|--luahttp

#注意事项
- 本库只做http协议解析
- 数据收发由调用方实现

#用法
```lua
local lhttp = require("luahttp")

function on_msg_recv(buf)
    local request = lhttp.create_request()
    request:append(buf)
    request:process()
    local body = request:body()
    local state = request:state()
    local target = request:target()
    local target = request:target()
    local is_chunk = request:is_chunk()
    local headers = request:headers()
    --快速返回
    local res_str = request:response(200, "application/json", "{code:123}")
    send_buf(res_str)
    request:close()
    --自定义返回
    local res = lhttp.create_response()
    res:set_status(200)
    res:set_header("key", "value")
    res:set_body("{code:123}")
    local res_str = res:respond(request)
    send_buf(res_str)
    res:close()
end

```

#备注
本库参考自https://github.com/jeremycw/httpserver.h。