# lhttp
一个提供给lua使用的http协议解析库。

# 依赖
- [lua](https://github.com/xiyoo0812/lua.git)5.3以上
- [luakit](https://github.com/xiyoo0812/luakit.git)一个luabind库
- 项目路径如下<br>
  |--proj <br>
  &emsp;|--lua <br>
  &emsp;|--lhttp <br>
  &emsp;|--luakit

# 注意事项
- 本库只做http协议解析
- 数据收发由调用方实现

# 用法
```lua
--本示例使用了quanta引擎
--https://github.com/xiyoo0812/quanta.git
--http_server.lua
local lhttp         = require("lhttp")
local ljson         = require("lcjson")
local Socket        = import("driver/socket.lua")

local type          = type
local tostring      = tostring
local getmetatable  = getmetatable
local log_err       = logger.err
local log_info      = logger.info
local log_debug     = logger.debug
local json_encode   = ljson.encode
local tunpack       = table.unpack
local signalquit    = signal.quit
local saddr         = string_ext.addr

local thread_mgr    = quanta.get("thread_mgr")

local HttpServer = class()
local prop = property(HttpServer)
prop:reader("listener", nil)        --网络连接对象
prop:reader("ip", nil)              --http server地址
prop:reader("port", 8080)           --http server端口
prop:reader("clients", {})          --clients
prop:reader("requests", {})         --requests
prop:reader("get_handlers", {})     --get_handlers
prop:reader("put_handlers", {})     --put_handlers
prop:reader("del_handlers", {})     --del_handlers
prop:reader("post_handlers", {})    --post_handlers

function HttpServer:__init(http_addr)
    self:setup(http_addr)
end

function HttpServer:setup(http_addr)
    self.ip, self.port = saddr(http_addr)
    local socket = Socket(self)
    if not socket:listen(self.ip, self.port) then
        log_info("[HttpServer][setup] now listen %s failed", http_addr)
        signalquit(1)
        return
    end
    log_info("[HttpServer][setup] listen(%s:%s) success!", self.ip, self.port)
    self.listener = socket
end

function HttpServer:on_socket_error(socket, token, err)
    if socket == self.listener then
        log_info("[HttpServer][on_socket_error] listener(%s:%s) close!", self.ip, self.port)
        self.listener = nil
        return
    end
    log_debug("[HttpServer][on_socket_error] client(token:%s) close!", token)
    self.clients[token] = nil
    self.requests[token] = nil
end

function HttpServer:on_socket_accept(socket, token)
    log_debug("[HttpServer][on_socket_accept] client(token:%s) connected!", token)
    self.clients[token] = socket
end

function HttpServer:on_socket_recv(socket, token)
    local request = self.requests[token]
    if not request then
        request = lhttp.create_request()
        log_debug("[HttpServer][on_socket_accept] create_request(token:%s)!", token)
        self.requests[token] = request
    end
    local buf = socket:get_recvbuf()
    if #buf == 0 or not request.parse(buf) then
        log_err("[HttpServer][on_socket_recv] http request append failed, close client(token:%s)!", token)
        self:response(socket, 400, "this http request parse error!")
        return
    end
    socket:pop(#buf)
    thread_mgr:fork(function()
        local method = request.method
        local headers = request.get_headers()
        if method == "GET" then
            local querys = request.get_params()
            return self:on_http_request(self.get_handlers, socket, request.url, querys, headers)
        end
        if method == "POST" then
            return self:on_http_request(self.post_handlers, socket, request.url, request.body, headers)
        end
        if method == "PUT" then
            return self:on_http_request(self.put_handlers, socket, request.url, request.body, headers)
        end
        if method == "DELETE" then
            local querys = request.get_params()
            return self:on_http_request(self.del_handlers, socket, request.url, querys, headers)
        end
    end)
end

--注册get回调
function HttpServer:register_get(url, handler, target)
    log_debug("[HttpServer][register_get] url: %s", url)
    self.get_handlers[url] = { handler, target }
end

--注册post回调
function HttpServer:register_post(url, handler, target)
    log_debug("[HttpServer][register_post] url: %s", url)
    self.post_handlers[url] = { handler, target }
end

--注册put回调
function HttpServer:register_put(url, handler, target)
    log_debug("[HttpServer][register_put] url: %s", url)
    self.put_handlers[url] = { handler, target }
end

--注册del回调
function HttpServer:register_del(url, handler, target)
    log_debug("[HttpServer][register_del] url: %s", url)
    self.del_handlers[url] = { handler, target }
end

--生成response
function HttpServer:build_response(status, content, headers)
    local response = lhttp.create_response()
    response.status = status
    response.content = content
    for name, value in pairs(headers or {}) do
        response.set_header(name, value)
    end
    return response
end

--http post 回调
function HttpServer:on_http_request(handlers, socket, url, ...)
    log_info("[HttpServer][on_http_request] request %s process!", url)
    local handler_info = handlers[url] or handlers["*"]
    if handler_info then
        local handler, target = tunpack(handler_info)
        if not target then
            if type(handler) == "function" then
                local ok, response = pcall(handler, url, ...)
                if not ok then
                    response = { code = 1, msg = response }
                end
                self:response(socket, 200, response)
                return
            end
        else
            if type(handler) == "string" then
                handler = target[handler]
            end
            if type(handler) == "function" then
                local ok, response = pcall(handler, target, url, ...)
                if not ok then
                    response = { code = 1, msg = response }
                end
                self:response(socket, 200, response)
                return
            end
        end
    end
    log_err("[HttpServer][on_http_request] request %s hasn't process!", url)
    self:response(socket, 404, "this http request hasn't process!")
end

function HttpServer:response(socket, status, response)
    local token = socket:get_token()
    if not token then
        return
    end
    self.requests[token] = nil
    if getmetatable(response) then
        socket:send(response.serialize())
        socket:close()
        return
    end
    local new_resp = lhttp.create_response()
    if type(response) == "table" then
        new_resp.set_header("Content-Type", "application/json")
        new_resp.content = json_encode(response)
    else
        new_resp.set_header("Content-Type", "text/plain")
        new_resp.content = (type(response) == "string") and response or tostring(response)
    end
    new_resp.status = status
    socket:send(new_resp.serialize())
    socket:close()
end

return HttpServer
```
