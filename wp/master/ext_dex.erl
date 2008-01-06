
-module(ext_dex).

-export([ext_init/2, ext_exit/1, iblock_registry_process/1]).

iblock_registry_process(S) ->
        ets:new(iblock_registry, [named_table, public]),
        S ! ok,
        receive
                _ -> ok
        end.

init_iblock_registry() ->
        case ets:info(iblock_registry) of
                undefined ->
                        spawn(ext_dex, iblock_registry_process, 
                                [self()]),
                        receive
                                _ -> ok
                        end;
                _ -> ok
        end.

ext_init("merger", Socket) ->
        error_logger:info_report(["merger", Socket]),
        init_iblock_registry(),
        N = [Normtable || {_Iblock, Normtable} 
                <- ets:tab2list(iblock_registry)],
        {ok, _} = erlay:communicate(Socket, ["add_normtable:", N]), ok;

ext_init(Iblock, Socket) ->
        error_logger:info_report(["new iblock", Socket]),
        init_iblock_registry(),
        error_logger:info_report(["req normtable"]),
        {ok, Normtable} = erlay:communicate(Socket, "get_normtable:"),
        error_logger:info_report(["ok, got normtable"]),
        ets:insert(iblock_registry, {Iblock, Normtable}),
        (catch gen_server:call(erlay,
                {ext_sync, "merger", ["add_normtable:", Normtable]}, 
                        1000)), ok.

ext_exit("merger") -> ok;

ext_exit(Iblock) ->
        ets:delete(iblock_registry, Iblock).



