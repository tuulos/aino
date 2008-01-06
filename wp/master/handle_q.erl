
-module(handle_q).

-export([handle/2, ctrl/2]).

-define(HTTP_HEADER, "HTTP/1.1 200 OK\n"
                     "Status: 200 OK\n"
                     "Content-type: text/html\n\n").

ctrl(Target, Msg) ->
	{ok, _} = gen_server:call(erlay, 
                {ext_sync, Target, ["_erlay:", Msg]}).

distribute_to_iblocks(Msg, CacheKey) ->
        error_logger:info_report(["DistrToIBlocks", CacheKey]),
        Iblocks = ets:tab2list(iblock_registry),
        lists:foreach(fun({Iblock, _}) ->
                ok = gen_server:call(erlay, 
                        {ext_async, Iblock, Msg, CacheKey})
                end, Iblocks),
        gen_server:call(erlay_cache, 
                {wait, CacheKey, length(Iblocks)}, 10000).

scatter_and_gather(CacheKey, DistrMsg, MergeCmd) ->
        error_logger:info_report(trunc_io:fprint([{"ScatGat", DistrMsg, MergeCmd}], 100)),
        {ok, _R} = gen_server:call(erlay_cache,
                {newentry, {CacheKey, "merger"}}),
        {ok, IblockRes} = distribute_to_iblocks(DistrMsg, {iblocks, CacheKey}),
        {ok, Merged} = gen_server:call(erlay, 
                {ext_sync, "merger", [MergeCmd, 
                        [D || {_, {_ExtClass, D}} <- IblockRes]]}, 10000),
        ok = gen_server:call(erlay_cache,
                {put, {CacheKey, "merger", Merged}}),
        Merged.
        
score([{_, {"merger", Scored}}], _KeysCues, _Cues) -> Scored;
score([], KeysCues, Cues) -> scatter_and_gather({scored, Cues}, 
        ["score:", KeysCues], "merge_scores:").

rank([{_, {"merger", Ranked}}], _KeysCues, _Keys, _Scored) -> Ranked;
rank([], KeysCues, Keys, Scored) -> scatter_and_gather({ranked, Keys},
                ["rank:", [KeysCues, Scored]], "merge_ranked:").

handle(Socket, Msg) ->
        {value, {_, Query}} = lists:keysearch("QUERY_STRING", 1, Msg),
        
        {ok, KeysCues} = gen_server:call(erlay, 
                {ext_sync, "merger", ["parse_query:", Query]}, 5000),
        
        M = netstring:decode_netstring_fd(binary_to_list(KeysCues)),
        {value, Cues} = lists:keysearch("cues", 1, M),
        {value, Keys} = lists:keysearch("keys", 1, M),
        {value, Mods} = lists:keysearch("mods", 1, M),
        
        ScoreCacheHit = gen_server:call(erlay_cache, {get, {scored, Cues}}),
        Scored = score(ScoreCacheHit, KeysCues, Cues),
        RankCacheHit = gen_server:call(erlay_cache, {get, {ranked, {Keys, Cues, Mods}}}),
        Ranked = rank(RankCacheHit, KeysCues, {Keys, Cues, Mods}, Scored),
        
        ok = gen_server:call(erlay, {ext_async, "render",
                ["render:", KeysCues, Scored, Ranked], {render, KeysCues}}),
        {ok, [{_Key, {"render", Results}}]} = gen_server:call(erlay_cache, 
                        {wait, {render, KeysCues}, 1}, 10000),
        
        gen_tcp:send(Socket, [?HTTP_HEADER, Results]).

