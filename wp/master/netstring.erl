-module(netstring).
-export([decode_netstring_fd/1]).

decode_netstring_fd(Msg) ->
        Len = string:sub_word(Msg, 1, 10),
        decode_next_pair(string:substr(Msg, length(Len) + 2, 
                list_to_integer(Len)), []).

decode_next_pair([], Lst) -> Lst;
decode_next_pair(Msg, Lst) ->
        {Msg1, Key} = decode_next_item(Msg),
        {Msg2, Val} = decode_next_item(Msg1),
        decode_next_pair(Msg2, [{Key, Val}|Lst]).

decode_next_item(Msg) ->
        Len = string:sub_word(Msg, 1),
        I = list_to_integer(Len),
        {string:substr(Msg, length(Len) + I + 3), 
                string:substr(Msg, length(Len) + 2, I)}.
