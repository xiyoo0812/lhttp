# lhttp
一个提供给lua使用的http协议解析库。

# 依赖
- [lua](https://github.com/xiyoo0812/lua.git)5.3以上
- 项目路径如下<br>
  |--proj <br>
  &emsp;|--lua <br>
  &emsp;|--lhttp
- [luaext](https://github.com/xiyoo0812/luaext.git) 集成了所有lua扩展库，建议使用或者参考。

# 注意事项
- 本库只做http协议解析
- 数据收发由调用方实现

# 用法
```lua
local lhttp = require("lhttp")

function on_msg_recv(buf)
    local request = lhttp.create_request()
    request:append(buf)
    request:process()
    local url = request:url()
    local body = request:body()
    local state = request:state()
    local querys = request:querys()
    local headers = request:headers()
    local is_chunk = request:is_chunk()
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

# 备注
本库参考自https://github.com/jeremycw/httpserver.h
