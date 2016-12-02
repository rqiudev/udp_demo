#!/bin/bash

cd `dirname $0`
record_day=`date -d "1 day ago" +'%Y-%m-%d'`

echo "SET odps.stage.mapper.split.size = 32; select '$record_day' as dt, tc.gwip, te.ip, te.city, tc.cnt15, tc.total, if(tc.total>0, round(tc.cnt15/tc.total*100, 2), 0.00) as rate15 from (select  tb.gwip as gwip, ta.log_source as log_source, sum(if(get_json_object(ta.content, '$.content.tags.callee_seq[4]')>15,1,0)) as cnt15,  sum(if(get_json_object(ta.content, '$.content.tags.callee_seq[4]')>=0,1,0)) as total from ods_uxinapp_rtpp_d ta join (select get_json_object(content, '$.content.tags.callee_pkg_addr[0]') as gwip, count(*) as cb from ods_uxinapp_rtpp_d where dt='$record_day' and get_json_object(content, '$.content.tags.callee_seq[4]')>15 and get_json_object(content, '$.content.tags.call_mode')=3  group by  get_json_object(content, '$.content.tags.callee_pkg_addr[0]') order by cb desc limit 1) tb on get_json_object(ta.content, '$.content.tags.callee_pkg_addr[0]') = tb.gwip and ta.dt='$record_day' group by ta.log_source, tb.gwip limit 50) tc join d_ecs_ip_rela td on td.inner_ip=tc.log_source join ods_ip_info_n te on te.ip=td.outer_ip order by rate15 desc limit 50;" | odpscmd


